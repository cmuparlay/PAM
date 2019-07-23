include ../Makeheader 

all: 	rt_test rs_test  rs_test_list

rt_test:	rt_test.cpp range_tree.h
	$(CC) $(CFLAGS) rt_test.cpp -o rt_test $(LFLAGS)

rs_test:	rs_test.cpp range_sweep.h ../common/sweep.h
	$(CC) $(CFLAGS) -fopenmp rs_test.cpp -o rs_test $(LFLAGS)

rs_test_list:	rs_test_list.cpp range_sweep_list.h ../common/sweep.h
	$(CC) $(CFLAGS) -fopenmp rs_test_list.cpp -o rs_test_list $(LFLAGS)

clean:
	rm -f rt_test rs_test rs_test_list
