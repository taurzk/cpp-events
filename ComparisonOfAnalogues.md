

# Events and delegates in C# #

C# has built-in support for event/delegate mechanism.

Delegate types are declared using the `delegate` keyword.
```
namespace MyNamespace
{
    public delegate void TestDelegate(object obj);
    public delegate float MathDelegate(float x);
    public delegate void ClickDelegate(object sender, EventArgs e);
}
```
All delegate types are classes implicitly inherited from the `System.MulticastDelegate` class:

![http://cpp-events.googlecode.com/svn/wiki/CSharpDelegateHierarchy.png](http://cpp-events.googlecode.com/svn/wiki/CSharpDelegateHierarchy.png)

Classes `System.Delegate` and `System.MulticastDelegate` are special classes that are used exclusively for deriving delegate types and cannot be used for deriving custom classes.

Class `System.Delegate` implements abstract delegate class that encapsulates one invokable element (a static function, an instance method bound to specific instance or a closure, which are implemented in C# as anonymous delegates).

Class `System.MulticastDelegate` implements abstract delegate class that contains list of elements that are invoked one by one. Internally it is implemented as a linked list of `System.Delegate` objects. All user-defined delegates are multicast delegates.

Multicast delegates can be concatenated or substracted using operations "+", "–", "+=" and "–=".

When multicast delegate is invoked, all its elements are invoked in the order of appending. If multicast delegate returns a value, the value, returned by the last element, is returned.

Delegate always contains at least one element. If list of elements becomes empty then delegate object is destroyed and variable, that used to reference that object, is set to null.

It is not valid to invoke delegate via null reference. Thus delegate reference should always be checked for null, before invoking delegate. This is a major inconvenience, that is often criticized by C# users. One of the possible reasons for such design is problem of undefined return value for empty non-void delegates, but this is just my personal assumption.

Event in C# conceptually is a pair of special functions that provide connection management interface. These functions can be virtual or abstract - in this way events can be overridden in derived classes and used in interfaces.

Events in C# are not objects, and thus do not have type property as variables do. Instead of this, event declaration specifies type of delegates that can be subscribed for this event. Signature of this delegate type is considered as a signature of the event itself.

Functions that implement event both have one argument of associated delegate type. This argument is declared implicitly, in functions' bodies it can be referenced using a `value` keyword.
```
class MyClass
{
    public event MyDelegateType myEvent
    {
        add
        {
            MyDelegateType localVar = value;
            // ...
        }
        
        remove
        {
            MyDelegateType localVar = value;
            // ...
        }
     }
}
```
Implementation of these functions directly or indirectly relies on some kind of delegate container - the `add` function adds new items to that container, and the `remove` function removes specified items from the container. Since delegates are containers themselves, than usually simple variable of delegate type is used for that container.

In the most common (and the most trivial) case event implementation relies on member variable of delegate type, and event functions do nothing but "+=" and "-=" operations on this variable. In this case event can be declared using special simplified form known as «Field-Like Events»:
```
    public event MyDelegateType myEvent;
```
This sentence declares two items named `myEvent`. First one is a member variable of type `MyDelegateType`, that provides underlying container for event implementation. Second one is an event itself, that consists from a pair of trivial in-line functions. The variable always has `private` access. Event functions have access level specified in event declaration statement. Arbitrary access level can be set for the variable when using complete event declaration syntax.

# Signals and slots in Qt #
# Boost.Signals #
# LibSigC++ #
# SigSlot #
# Boost.Signals2 #