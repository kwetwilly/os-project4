all: site-tester

site-tester: site-tester.cpp
	/afs/nd.edu/user14/csesoft/new/bin/g++ site-tester.cpp -Wall -std=c++11 -o site-tester -lcurl -pthread -static-libstdc++

clean:
	rm -f site-tester *.o