include ../Makeheader 

all: rec_sweep rec_test rec_sweep2 rec_test2

rec_test:	rec_test.cpp rectangle.h
	$(CC) $(CFLAGS) rec_test.cpp -o rec_test $(LFLAGS)

rec_sweep:	rec_sweep.cpp rec_sweep.h
	$(CC) $(CFLAGS) rec_sweep.cpp -o rec_sweep $(LFLAGS)

rec_test2:	rec_test2.cpp rectangle2.h
	$(CC) $(CFLAGS) rec_test2.cpp -o rec_test2 $(LFLAGS)

rec_sweep2:	rec_sweep2.cpp rec_sweep2.h
	$(CC) $(CFLAGS) rec_sweep2.cpp -o rec_sweep2 $(LFLAGS)


clean:
	rm -f rec_sweep rec_test rec_sweep2 rec_test2
