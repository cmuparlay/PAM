CC = g++

ifeq ($(shell uname -s),Darwin)
CFLAGS = -O2 -I../c++ -mcx16 -march=native -std=c++17 -Wall
else
CFLAGS = -O2 -I../c++ -mcx16 -march=native -DCILK -fcilkplus -std=c++11 -Wall
#CFLAGS = -O2 -I../c++ -mcx16 -march=native -DHOMEGROWN -fopenmp -std=c++11 -Wall
endif

all:	testParallel testParallelNA unit_tests

testParallel:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG testParallel.cpp -o testParallel

testParallelNA:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG -DNO_AUG testParallel.cpp -o testParallelNA

unit_tests:	unit_tests.cpp
	$(CC) $(CFLAGS) unit_tests.cpp -o unit_tests

clean:
	rm -f testParallel testParallelNA unit_tests
