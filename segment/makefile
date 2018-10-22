CC = g++ 
CFLAGS = -O2 -DNDEBUG -mcx16 -g -I../c++ -march=native -fcilkplus -lcilkrts -std=c++11

# Compiler flags for the TBB library. Note tbb library currently not present in repo.
# CFLAGS = -O2 -I../c++ -I../tbb/include/ -L../tbb/lib/intel64/gcc4.4 -ltbbmalloc -fopenmp -fcilkplus -lcilkrts -std=c++11
all: seg_test seg_test_star seg_sweep seg_sweep_star

seg_test:	seg_test.cpp segment.h
	$(CC) $(CFLAGS) seg_test.cpp -o seg_test

seg_sweep:	seg_sweep.cpp seg_sweep.h
	$(CC) $(CFLAGS) seg_sweep.cpp -o seg_sweep
	
seg_sweep_star:	seg_sweep_star.cpp seg_sweep_star.h
	$(CC) $(CFLAGS) seg_sweep_star.cpp -o seg_sweep_star

seg_test_star:	seg_test_star.cpp segment_star.h
	$(CC) $(CFLAGS) seg_test_star.cpp -o seg_test_star
	
clean:
	rm -f seg_test seg_sweep seg_test_star seg_sweep_star
