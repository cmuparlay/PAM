include ../../Makeheader 

Queries = queries/queries.h queries/Q1.h queries/Q2.h queries/Q3.h queries/Q4.h queries/Q5.h queries/Q6.h queries/Q7.h queries/Q8.h queries/Q9.h queries/Q10.h queries/Q11.h queries/Q12.h queries/Q13.h queries/Q14.h queries/Q15.h queries/Q16.h queries/Q17.h queries/Q18.h queries/Q19.h queries/Q20.h queries/Q21.h queries/Q22.h

test:	lineitem.h tables.h utils.h test.cpp new_orders.h $(Queries)
	$(CC) $(CFLAGS) -w test.cpp -o test $(LFLAGS)

runtest20: test
	NUM_THREADS=20 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest1: test
	NUM_THREADS=1 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest64: test
	NUM_THREADS=64 numactl -i all -C 0-32,72-104 ./test -q -s 100

clean:
	rm -f test
