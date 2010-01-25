#include <Cpp/Events.hpp>
#include <gtest/gtest.h>

class Sender
{
public:
	void fire() { somethingHappened_.fire(); }
	Cpp::EventRef<> somethingHappened() { return somethingHappened_; }
private:
	Cpp::Event<> somethingHappened_;
};

class Reciever
{
public:
	Reciever() : val_() {}

	void increment() { ++val_; }
	void decrement() { --val_; }
	int value() const { return val_; }
	void setValue(int v) { val_ = v; }
private:
	int val_;
};

class SenderEx : public Sender
{
public:
	SenderEx()
	{
		stageNo_ = 0; // 1 2 3 4 ...
		stageStep_ = 1; // 2 4 8 16 ...
	}

	int stageNo() const { return stageNo_; }
	int stageStep() const { return stageStep_; }

	void runStage()
	{
		fire();
		++stageNo_;
		stageStep_ *= 2;
	}
private:
	int stageNo_;
	int stageStep_;
};

template<int ArraySize> class RecieverEx
{
public:
	RecieverEx()
		: sender_()
		, scope_()
		, index_(-1)
		, val_()
	{}

	int index() const { return index_; }
	int value() const { return val_; }

	void connect(int ind, SenderEx * sender, Cpp::ConnectionScope * scope)
	{
		index_ = ind;
		sender_ = sender;
		scope_ = scope;
		scope->connect(sender->somethingHappened(), this, &RecieverEx<ArraySize>::work);
	}

	void work()
	{
		++val_;

		int step = sender_->stageStep();
		int nextIndex = index_ + step;
		if(nextIndex < ArraySize)
		{
			RecieverEx<ArraySize> * next = this + step;
			next->connect(nextIndex, sender_, scope_);
		}
	}
private:
	SenderEx * sender_;
	Cpp::ConnectionScope * scope_;
	int index_;
	int val_;
};

////////////////////////////////////////////////////////////////////////////////
// This test checks basic connection management
TEST(Test_ConnectDisconnect, ManualConnectDisconnect)
{
	Sender sender;
	Reciever r1, r2;
	Cpp::ConnectionScope scope;

	ASSERT_EQ(0, r1.value()); ASSERT_EQ(0, r2.value());
	sender.fire();
	ASSERT_EQ(0, r1.value()); ASSERT_EQ(0, r2.value());
	
	scope.connect(sender.somethingHappened(), &r1, &Reciever::increment);
	sender.fire();	// +1-0 +0-0
	ASSERT_EQ(1, r1.value()); ASSERT_EQ(0, r2.value());
	sender.fire();	// +1-0 +0-0
	ASSERT_EQ(2, r1.value()); ASSERT_EQ(0, r2.value());
	
	scope.connect(sender.somethingHappened(), &r2, &Reciever::increment);
	sender.fire();	// +1-0 +1-0
	ASSERT_EQ(3, r1.value()); ASSERT_EQ(1, r2.value());

	scope.connect(sender.somethingHappened(), &r1, &Reciever::decrement);
	sender.fire();	// +1-1 +1-0
	ASSERT_EQ(3, r1.value()); ASSERT_EQ(2, r2.value());

	scope.connect(sender.somethingHappened(), &r1, &Reciever::decrement);
	sender.fire();	// +1-2 +1-0
	ASSERT_EQ(2, r1.value()); ASSERT_EQ(3, r2.value());

	scope.connect(sender.somethingHappened(), &r2, &Reciever::decrement);
	sender.fire();	// +1-2 +1-1
	ASSERT_EQ(1, r1.value()); ASSERT_EQ(3, r2.value());

	scope.connect(sender.somethingHappened(), &r2, &Reciever::increment);
	sender.fire();	// +1-2 +2-1
	ASSERT_EQ(0, r1.value()); ASSERT_EQ(4, r2.value());

	sender.somethingHappened().disconnectAll(&r2, &Reciever::decrement);
	sender.fire();	// +1-2 +2-0
	ASSERT_EQ(-1, r1.value()); ASSERT_EQ(6, r2.value());

	sender.somethingHappened().disconnectOne(&r1, &Reciever::decrement);
	sender.fire();	// +1-1 +2-0
	ASSERT_EQ(-1, r1.value()); ASSERT_EQ(8, r2.value());

	sender.somethingHappened().disconnectAll(&r1, &Reciever::increment);
	sender.fire();	// +0-1 +2-0
	ASSERT_EQ(-2, r1.value()); ASSERT_EQ(10, r2.value());

	sender.somethingHappened().disconnectOne(&r2, &Reciever::increment);
	sender.fire();	// +0-1 +1-0
	ASSERT_EQ(-3, r1.value()); ASSERT_EQ(11, r2.value());

	sender.somethingHappened().disconnectAll();
	sender.fire();	// +0-0 +0-0
	ASSERT_EQ(-3, r1.value()); ASSERT_EQ(11, r2.value());
}

////////////////////////////////////////////////////////////////////////////////
// This test checks automatic disconnection
TEST(Test_ConnectDisconnect, AutomaticDisconnect)
{
	Cpp::ConnectionScope scope0;
	Reciever r0;
	{
		Sender sender;

		sender.fire();
		{
			Reciever r1;
			{
				Cpp::ConnectionScope scope1;
				scope1.connect(sender.somethingHappened(), &r1, &Reciever::increment);
				ASSERT_EQ(0, r1.value());
				sender.fire();
				ASSERT_EQ(1, r1.value());
			}
			sender.fire();
			ASSERT_EQ(1, r1.value());
			{
				Cpp::ConnectionScope scope2;
				scope2.connect(sender.somethingHappened(), &r1, &Reciever::decrement);
				ASSERT_EQ(1, r1.value());
				sender.fire();
				ASSERT_EQ(0, r1.value());
			}
			sender.fire();
			ASSERT_EQ(0, r1.value());

			{
				scope0.connect(sender.somethingHappened(), &r0, &Reciever::setValue, 5);
				sender.fire();
				ASSERT_EQ(5, r0.value());
				r0.setValue(-1);
				sender.fire();
				ASSERT_EQ(5, r0.value());
			}
		}
		r0.setValue(-1);
		sender.fire();
		ASSERT_EQ(5, r0.value());
		ASSERT_NE(0, scope0.connectionCount());
	}
	ASSERT_EQ(0, scope0.connectionCount());
}


////////////////////////////////////////////////////////////////////////////////
// This test ensures that adding connections inside delegate works fine.
TEST(Test_ConnectDisconnect, ConnectFromDelegate)
{
	RecieverEx<8> rcv[8];
	Cpp::ConnectionScope scope;
	SenderEx sender;

	rcv[0].connect(0, &sender, &scope);

	ASSERT_EQ(0, rcv[0].value());
	ASSERT_EQ(0, rcv[1].value());
	ASSERT_EQ(0, rcv[2].value());
	ASSERT_EQ(0, rcv[3].value());
	ASSERT_EQ(0, rcv[4].value());
	ASSERT_EQ(0, rcv[5].value());
	ASSERT_EQ(0, rcv[6].value());
	ASSERT_EQ(0, rcv[7].value());
	ASSERT_EQ(1, sender.somethingHappened().connectionCount());
	
	sender.runStage();
	ASSERT_EQ(1, rcv[0].value());
	ASSERT_EQ(0, rcv[1].value());
	ASSERT_EQ(0, rcv[2].value());
	ASSERT_EQ(0, rcv[3].value());
	ASSERT_EQ(0, rcv[4].value());
	ASSERT_EQ(0, rcv[5].value());
	ASSERT_EQ(0, rcv[6].value());
	ASSERT_EQ(0, rcv[7].value());
	ASSERT_EQ(2, sender.somethingHappened().connectionCount());

	sender.runStage();
	ASSERT_EQ(2, rcv[0].value());
	ASSERT_EQ(1, rcv[1].value());
	ASSERT_EQ(0, rcv[2].value());
	ASSERT_EQ(0, rcv[3].value());
	ASSERT_EQ(0, rcv[4].value());
	ASSERT_EQ(0, rcv[5].value());
	ASSERT_EQ(0, rcv[6].value());
	ASSERT_EQ(0, rcv[7].value());
	ASSERT_EQ(4, sender.somethingHappened().connectionCount());

	sender.runStage();
	ASSERT_EQ(3, rcv[0].value());
	ASSERT_EQ(2, rcv[1].value());
	ASSERT_EQ(1, rcv[2].value());
	ASSERT_EQ(1, rcv[3].value());
	ASSERT_EQ(0, rcv[4].value());
	ASSERT_EQ(0, rcv[5].value());
	ASSERT_EQ(0, rcv[6].value());
	ASSERT_EQ(0, rcv[7].value());
	ASSERT_EQ(8, sender.somethingHappened().connectionCount());

	sender.runStage();
	ASSERT_EQ(4, rcv[0].value());
	ASSERT_EQ(3, rcv[1].value());
	ASSERT_EQ(2, rcv[2].value());
	ASSERT_EQ(2, rcv[3].value());
	ASSERT_EQ(1, rcv[4].value());
	ASSERT_EQ(1, rcv[5].value());
	ASSERT_EQ(1, rcv[6].value());
	ASSERT_EQ(1, rcv[7].value());
	ASSERT_EQ(8, sender.somethingHappened().connectionCount());
}
