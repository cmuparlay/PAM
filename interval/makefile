CC = g++

ifeq ($(shell uname -s),Darwin)
CFLAGS = -O2 -I../c++ -mcx16 -DHOMEGROWN -march=native -std=c++17 -Wall
else
#CFLAGS = -O2 -I../c++ -mcx16 -march=native -DCILK -fcilkplus -std=c++11 -Wall
CFLAGS = -O2 -I../c++ -mcx16 -march=native -DHOMEGROWN -fopenmp -std=c++11 -Wall
endif

intervalTree:	intervalTree.cpp
	$(CC) $(CFLAGS) -DNDEBUG intervalTree.cpp -o intervalTree

clean:
	rm -f intervalTree
