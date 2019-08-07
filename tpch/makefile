include ../Makeheader 

test:	lineitem.h tables.h utils.h readCSV.h test.cpp queries.h Q1.h Q2.h Q3.h Q4.h Q5.h Q6.h Q7.h Q8.h Q9.h Q10.h Q11.h Q12.h Q13.h Q14.h Q15.h Q16.h Q17.h Q18.h Q19.h Q20.h Q21.h Q22.h
	$(CC) $(CFLAGS) -w test.cpp -o test $(LFLAGS)

runtest20: test
	NUM_THREADS=20 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest1: test
	NUM_THREADS=1 numactl --cpunodebind=0 --membind=0 ./test -q -s 10

runtest64: test
	NUM_THREADS=64 numactl -i all -C 0-32,72-104 ./test -q -s 100

clean:
	rm -f test
