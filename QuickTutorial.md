

# Initialization #

Library stores some data in thread-local storage and this data must be initialized before library can be used. Per-process initialization and deinitialization is done by creating instance of the class `Cpp::Events::ProcessInit` on the stack of the `main()` function. This class does nothing more than invoking static function `Cpp::Events::constructProcessData()` in constructor and `Cpp::Events::destructProcessData()` in destructor, but it is preferable to use `Cpp::Events::ProcessInit` class instead of calling these functions directly, because it is exception-safe.

**Example 1.** Per-process library initialization.
```
#include <Cpp/Events.hpp>

int main(int argc, char * argv[])
{
    Cpp::Events::ProcessInit processInit;

    // ...

    return 0;
}
```

For threads other than the main one, library must be initialized separately. The `Cpp::Events::ThreadInit` class is used or this. In the same way as `Cpp::Events::ProcessInit` does, this class does nothing more than invoking static function `Cpp::Events::constructThreadData()` in constructor and `Cpp::Events::destructThreadData()` in destructor.

**Example 2.** Per-thread library initialization.
```
void myThreadProc(void * threadParam)
{
    Cpp::Events::ThreadInit threadInit;
    
    // all the thread stuff goes here ...

}
```

# Declaring Events #

Event object has two interfaces - one for invocation and another for managing connections. The first one is used by sender object to perform notifications. The second one is used by receiver objects (or by the third party) to connect and disconnect event handlers.

Unlike Boost.Signals and other template-based implementations, Cpp::Events separates these two interfaces. So event in Cpp::Events implementation is represented by the two entities.

The first one is a member variable of template class `Cpp::Event<>`. The second one is a member function that takes no arguments and returns temporary object of class `Cpp::EventRef<>` constructed from a reference to that variable. The variable is an event implementation itself. The function is a provider of the connection management interface. To draw an analogy with C#, the function is an event itself, and the variable is an underlying multicast delegate.

**Example 3.** Button class with 'clicked' event.
```
class Button : public Widget
{
public:
    Button(Widget * parentWidget);

    // Event connection interface
    Cpp::EventRef<> clicked() { return clicked_; }
protected:
    virtual void mouseDown(MouseParams const & mouseParams);
private:
    // Event implementation
    Cpp::Event clicked_;
};

void Button::mouseDown(MouseParams const & mouseParams)
{
    if(mouseParams.button() == MouseParams::LeftButton)
    {
        clicked_.fire();
    }
}
```

# Connecting Events #

Events are connected to event handlers. In Cpp::Events event handlers are member functions bound to the specific object. Such pair is called a delegate. For some reasons there are no built-in delegates in C++. Internally Cpp::Events uses hackish [delegate library](http://www.codeproject.com/KB/cpp/FastDelegate.aspx) by Don Clugston, but when making connections pointer to object and pointer to member function are passed as separate values.

Event is connected to delegate with the help of the object of `Cpp::ConnectionScope` class. That object determines lifetime of the established connection - when connection scope object dies all related connections are automatically broken.

Connection is established by `connect()` method of the `Cpp::ConnectionScope`. It is overloaded function that requires at least three arguments. The first one is the `Cpp::EventRef<>` object, the second one is a pointer to the receiver object and the third one is a pointer to the member of the receiver object.

**Example 4.** Dialog class with with 'Ok' and 'Cancel' buttons.
```
class Dialog : public Widget
{
public:
    Dialog(Widget * parentWidget)
        : Widget(parentWidget)
    {
        okButton_.reset(new Button(this));
        okButton_->setGeometry(100, 200, 100, 50);
        scope_.connect(okButton_->clicked(), this, &Dialog::accept);
        
        cancelButton_.reset(new Button(this));
        cancelButton_->setGeometry(300, 200, 100, 50);
        scope_.connect(cancelButton_->clicked(), this, &Dialog::reject);
    }

private:
    std::auto_ptr<Button> okButton_;
    std::auto_ptr<Button> cancelButton_;
    Cpp::ConnectionScope scope_;

    void accept();
    void reject();
};
```

# Arguments #

Events can have arguments. Types of arguments are specified as arguments for template classes `Cpp::Event<>` and `Cpp::EventRef<>`.

**Example 5.** Checkbox class.
```
class CheckBox : public Widget
{
public:
   CheckBox(Widget * parentWidget);

   bool isChecked() const { return isChecked_; }
   void setChecked(bool b);

   Cpp::EventRef<bool> toggled() { return toggled_; }
private:
   bool isChecked_;
   Cpp::Event<bool> toggled_;
};

void CheckBox::setChecked(bool b)
{
    isChecked_ = b;
    repaint();
    toggled_.fire(b);
}
```

No type transformations are applied to the argument types specified in the declaration. If argument should be passed by const reference instead of copying it, then const reference type should be used in `Cpp::Event<>` and `Cpp::EventRef<>`.

**Example 6.** Combo box class.
```
class ComboBox : public Widget
{
public:
    ComboBox(Widget * parentWidget);

    void appendItem(String const & item);
    void insertItem(int index, String const & item);
    void removeItem(int index);
    int findItem(String const & item);

    int currentItem() const;
    void setCurrentItem(int i);

    Cpp::EventRef<int, String const &> currentItemChanged() { return currentItemChanged_; }
private:
    std::vector<String> items_;
    int currentItemIndex_;
    Cpp::Event<int, String const &> currentItemChanged_;
};

void ComboBox::setCurrentItem(int i)
{
    currentItemIndex_ = i;
    repaint();
    currentItemChanged_.fire(i, items_[i]);
}
```

# Signature Transformation #

## Parametrization ##

In some cases one event handler can be connected to several different events and event handler should distinguish them. One possible solution is to provide event handler with a pointer to the event object itself or to the object that contains the event. But CppEvents uses more flexible approach. It allows to use values of arbitrary types as an identifier of the event source. These values are passed to the event handler as left-most arguments.

The main benefit of this approach is that parametrization is performed in the terms of the event handler, not the event source.

**Note!** Parameters are assigned with the connection. If parameters are objects, then they will be destructed when connection is broken. And this may occur in the context of the different thread. Avoid using objects with thread-affinity as connection parameters.

**Example 7.** View with sorting menu.
```
class AbstractModel
{
public:
    enum SortingMethod
    {
        SortByName,
        SortBySize,
        SortByDate,
        SortingMethodCount
    };

    // ...
    
    virtual void sort(SortingMethod method) = 0;

    // ...
};

class ViewWithMenu : public Widget, public AbstractView
{
public:
    ViewWithMenu(Widget * parentWidget, AbstractModel * model)
        : Widget(parentWidget)
        , model_(model)
    {
        menu_.reset(new Menu(this, "Sorting"));
        {
            MenuItem * item = new MenuItem("&Name");
            scope_.connect(item->activated(), model_, &AbstractModel::sort, AbstractModel::SortByName);
            menu_->insertItem(item);
        }
        {
            MenuItem * item = new MenuItem("&Size");
            scope_.connect(item->activated(), model_, &AbstractModel::sort, AbstractModel::SortBySize);
            menu_->insertItem(item);
        }
        {
            MenuItem * item = new MenuItem("&Date");
            scope_.connect(item->activated(), model_, &AbstractModel::sort, AbstractModel::SortByDate);
            menu_->insertItem(item);
        }
    }
private:
    std::auto_ptr<Menu> menu_;
    AbstractModel * model_;
    Cpp::ConnectionScope scope_;       
};

```

## Argument Conversion ##

Event and event handler may have different (but convertible) types of arguments. In this case special wrapper is created automatically during connection.

**Example 8.** Example of argument conversion.
```

class SenderClass
{
public:
    Cpp::EventRef<int> intEvent() { return intEvent_; }
    Cpp::EventRef<String const &> stringEvent() { return stringEvent_; }
private:
    Cpp::Event<int> intEvent_;
    Cpp::Event<String const &> stringEvent_;
};

class RecieverClass
{
public:
    void intHandler(unsigned);
    void stringCopyHandler(String);
    void stringHandler(std::string const &);
};

class ConnectorClass
{
public:
    ConnectorClass()
    {
        // "int" to "unsigned" conversion
        scope_.connect(sender_.intEvent(), &reciever_, &RecieverClass::intHandler);
        // "String const &" to "String" conversion
        scope_.connect(sender_.stringEvent(), &reciever_, &RecieverClass::stringCopyHandler);
        // "String const &" to "std::string const &" conversion
        scope_.connect(sender_.stringEvent(), &reciever_, &RecieverClass::stringHandler);
    }
private:
    SenderClass sender_;
    RecieverClass reciever_;
    Cpp::ConnectionScope scope_;
};

```

## Discarding Arguments ##

Event can have more arguments then event handler requires. In this case some right-most event arguments can be discarded.

**Example 9.** Form that handles combo box event discarding second argument.
```
class FormWithComboBox : public Widget
{
public:
    FormWithComboBox(Widget * parentWidget)
        : Widget(parentWidget)
    {
        comboBox_.reset(new ComboBox(this));
        // Second argument will be discarded
        scope_.connect(comboBox_->currentItemChanged(), this, &FormWithComboBox::comboBoxItemChanged);
    }
private:
    std::auto_ptr<ComboBox> comboBox_;
    Cpp::ConnectionScope scope_;

    void comboBoxItemChanged(int itemNo);
};
```

# Event Chaining #

Event handler can be method of any class and event classes are not an exception. The `fire()` method of the `Cpp::Event<>` class can be used as an event handler as well. This means that one event can be connected to the another. This technique is called _event chaining_.

**Example 10.** Find options dialog with case sensitivity check box.
```
class FindOptionsDialog : public Widget
{
public:
    FindOptionsDialog(Widget * parentWidget)
        : Widget(parentWidget)
    {
        caseSensitivity_.reset(new CheckBox(this));
        scope_.connect(caseSensitivity_->toggled(), &caseSensitivityChanged_, &Cpp::Event<bool>::fire);
    }

    Cpp::EventRef<bool> caseSensitivityChanged() { return caseSensitivityChanged_; }
private:
    std::auto_ptr<CheckBox> caseSensitivity_;
    Cpp::Event<bool> caseSensitivityChanged_;
    Cpp::ConnectionScope scope_;
};
```

Similar effect can be achieved by reusing connection interface of the original event.

**Example 11.** Find options dialog with case sensitivity check box (version 2).
```
class FindOptionsDialog : public Widget
{
public:
    FindOptionsDialog(Widget * parentWidget)
        : Widget(parentWidget)
    {
        caseSensitivity_.reset(new CheckBox(this));
    }

    Cpp::EventRef<bool> caseSensitivityChanged() { return caseSensitivity_->toggled(); }
private:
    std::auto_ptr<CheckBox> caseSensitivity_;
};
```

# Virtual Events #

Function that provides connection management interface for the event can be virtual and abstract (pure virtual) as any other instance member function. This effectively makes event virtual or abstract. This allows events to be used in interface classes (i.e. classes containing only pure virtual functions and no data).

**Example 12.** Worker thread interface.
```
class IWorkerThread
{
protected:
    IWorkerThread() {}
    virtual ~IWorkerThread() {}
public:
    virtual void start() = 0;
    virtual void cancel() = 0;

    Cpp::EventRef<float> progressChanged() = 0;
    Cpp::EventRef<> finished() = 0;
};
```

Virtual events can be redefined in derived classes.

**Example 13.** Event redefinition example.
```
class BaseClass
{
public:
    virtual Cpp::EventRef<int> somethingHappened() { return somethingHappened_; }
private:
    Cpp::Event<int> somethingHappened_;
};

class DerivedClass : public BaseClass
{
public:
    DerivedClass()
    {
        scope_.connect(BaseClass::somethingHappened(), this, &DerivedClass::somethingHappenedHook);
    }
    
    virtual Cpp::EventRef<int> somethingHappened() { return somethingHappened_; }
private:
    Cpp::Event<int> somethingHappened_;
    Cpp::ConnectionScope scope_;

    bool filterOutFunction(int value) const;

    void somethingHappenedHook(int value)
    {
        if(!filterOutFunction(value))
        {
            somethingHappened_.fire(value);
        }
    }
};
```