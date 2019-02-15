
ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -fcilkplus -ldl -std=c++17 -march=native
else
CC = g++
# works with g++ (GCC) 5.4.1
CFLAGS = -O2 -I../c++ -mcx16 -march=native -std=c++17 -Wall
#CFLAGS = -O2 -I../c++ -mcx16 -march=native -DCILK -fcilkplus -std=c++11 -Wall
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
