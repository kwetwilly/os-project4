all: site-tester

site-tester: site-tester.cpp
	g++ site-tester.cpp -Wall -o site-tester -lcurl

clean:
	rm -f site-tester *.o