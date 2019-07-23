include ../Makeheader 

all:	testParallel testParallelNA unit_tests

testParallel:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG testParallel.cpp -o testParallel $(LFLAGS)

testParallelNA:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG -DNO_AUG testParallel.cpp -o testParallelNA $(LFLAGS)

unit_tests:	unit_tests.cpp
	$(CC) $(CFLAGS) unit_tests.cpp -o unit_tests $(LFLAGS)

clean:
	rm -f testParallel testParallelNA unit_tests
