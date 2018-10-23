CC = g++ 
CFLAGS = -O2 -DNDEBUG -mcx16 -g -I../c++ -march=native -fcilkplus -lcilkrts -std=c++11

# Compiler flags for the TBB library. Note tbb library currently not present in repo.
# CFLAGS = -O2 -I../c++ -I../tbb/include/ -L../tbb/lib/intel64/gcc4.4 -ltbbmalloc -fopenmp -fcilkplus -lcilkrts -std=c++11
all: rec_sweep rec_test rec_sweep2 rec_test2

rec_test:	rec_test.cpp rectangle.h
	$(CC) $(CFLAGS) rec_test.cpp -o rec_test

rec_sweep:	rec_sweep.cpp rec_sweep.h
	$(CC) $(CFLAGS) rec_sweep.cpp -o rec_sweep
	
rec_test2:	rec_test2.cpp rectangle2.h
	$(CC) $(CFLAGS) rec_test2.cpp -o rec_test2

rec_sweep2:	rec_sweep2.cpp rec_sweep2.h
	$(CC) $(CFLAGS) rec_sweep2.cpp -o rec_sweep2


clean:
	rm -f rec_sweep rec_test rec_sweep2 rec_test2
