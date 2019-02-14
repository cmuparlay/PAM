ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -DCILK -fcilkplus -ldl -std=c++11 -march=native
else
CC = g++
# works with g++ (GCC) 5.4.1
#CFLAGS = -O2 -I../c++ -mcx16 -march=native -DCILK -fcilkplus -std=c++11 -Wall
CFLAGS = -O2 -I../c++ -mcx16 -march=native -DHOMEGROWN -fopenmp -std=c++11 -Wall
endif

intervalTree:	intervalTree.cpp
	$(CC) $(CFLAGS) -DNDEBUG intervalTree.cpp -o intervalTree

clean:
	rm -f intervalTree
