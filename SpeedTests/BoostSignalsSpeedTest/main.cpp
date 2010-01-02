#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <ctime>
#include <stdio.h>

class MyReciever : public boost::signals::trackable
{
public:
	void processFloat(float x) {}
	void processDouble(float x) {}
};

void test(unsigned N)
{
	boost::signal1<void,float> s1;
	boost::signal1<void,float> s2;

	boost::function1<void, float> f1;

	bool ok = (f1 == (&s2));

	s1.connect(s2);
	MyReciever rcv;

	unsigned X1 = 0, X2 = 0, X3 = 0;

	for(int j=0; j<8; ++j)
	{
		clock_t c0 = clock();

		for(unsigned i = 0; i< N; ++i)
		{
			s1.connect(boost::bind(&MyReciever::processFloat, &rcv, _1));
		}

		clock_t c1 = clock();

		for(int k=0; k<100; ++k)
		{
			s1(3.14f);
		}

		clock_t c2 = clock();

		s1.disconnect_all_slots();

		clock_t c3 = clock();

		X1 += (unsigned)(c1 - c0);
		X2 += (unsigned)(c2 - c1);
		X3 += (unsigned)(c3 - c2);
	}
	printf("%8u\t%06u\t%06u\t%06u\n", N, X1, X2, X3);
}

int main()
{
	printf("Boost.Signals speed test\n");
	for(unsigned a = 1000; a < 100000; )
	{
		unsigned x = a;
		for(unsigned i=0; i<9; ++i)
		{
			test(x);
			x += a;
		}
		a = x;
	}
	return 0;
}

