
ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -fcilkplus -ldl -std=c++11 -march=native
else
CC = g++
# works with g++ (GCC) 5.4.1
CFLAGS = -O2 -I../c++ -mcx16 -march=native -fcilkplus -std=c++11
endif

all:	aug_sum aug_sumNA

aug_sum:		aug_sum.cpp
	$(CC) $(CFLAGS) -DNDEBUG aug_sum.cpp -o aug_sum

aug_sumNA:	aug_sum.cpp
	$(CC) $(CFLAGS) -DNDEBUG -DNO_AUG aug_sum.cpp -o aug_sumNA



clean:
	rm -f aug_sum aug_sumNA
