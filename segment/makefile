include ../Makeheader 

all: seg_test seg_test2 seg_sweep seg_sweep2

g:	seg_test_g seg_sweep_g

seg_test:	seg_test.cpp segment.h
	$(CC) $(CFLAGS) seg_test.cpp -o seg_test $(LFLAGS)

seg_sweep:	seg_sweep.cpp seg_sweep.h
	$(CC) $(CFLAGS) seg_sweep.cpp -o seg_sweep $(LFLAGS)

seg_sweep2:	seg_sweep2.cpp seg_sweep2.h
	$(CC) $(CFLAGS) seg_sweep2.cpp -o seg_sweep2 $(LFLAGS)

seg_test2:	seg_test2.cpp segment2.h
	$(CC) $(CFLAGS) seg_test2.cpp -o seg_test2 $(LFLAGS)

seg_test_g:	seg_test_g.cpp segment_g.h
	$(CC) $(CFLAGS) seg_test_g.cpp -o seg_test_g $(LFLAGS) 

seg_sweep_g:	seg_sweep_g.cpp seg_sweep_g.h
	$(CC) $(CFLAGS) seg_sweep_g.cpp -o seg_sweep_g $(LFLAGS)

clean:
	rm -f seg_test seg_sweep seg_test2 seg_sweep2 seg_test_g seg_sweep_g
