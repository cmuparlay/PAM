CC = g++ 
CFLAGS = -O2 -DNDEBUG -mcx16 -g -I../c++ -march=native -DCILK -fcilkplus -lcilkrts -std=c++11

# Compiler flags for the TBB library. Note tbb library currently not present in repo.
# CFLAGS = -O2 -I../c++ -I../tbb/include/ -L../tbb/lib/intel64/gcc4.4 -ltbbmalloc -fopenmp -fcilkplus -lcilkrts -std=c++11
all: seg_test seg_test2 seg_sweep seg_sweep2

g:	seg_test_g seg_sweep_g

seg_test:	seg_test.cpp segment.h
	$(CC) $(CFLAGS) seg_test.cpp -o seg_test

seg_sweep:	seg_sweep.cpp seg_sweep.h
	$(CC) $(CFLAGS) seg_sweep.cpp -o seg_sweep
	
seg_sweep2:	seg_sweep2.cpp seg_sweep2.h
	$(CC) $(CFLAGS) seg_sweep2.cpp -o seg_sweep2

seg_test2:	seg_test2.cpp segment2.h
	$(CC) $(CFLAGS) seg_test2.cpp -o seg_test2
	
seg_test_g:	seg_test_g.cpp segment_g.h
	$(CC) $(CFLAGS) seg_test_g.cpp -o seg_test_g

seg_sweep_g:	seg_sweep_g.cpp seg_sweep_g.h
	$(CC) $(CFLAGS) seg_sweep_g.cpp -o seg_sweep_g

clean:
	rm -f seg_test seg_sweep seg_test2 seg_sweep2 seg_test_g seg_sweep_g
