all: site-tester

site-tester: site-tester.cpp
	g++ site-tester.cpp -Wall -std=c++11 -o site-tester -lcurl -pthread -static-libstdc++

clean:
	rm -f site-tester *.o