include ../Makeheader 

all:	testParallel testParallelNA unit_tests

testmi:	test_mi.cpp
	$(CC) $(CFLAGS) -DNDEBUG test_mi.cpp -o test_mi $(LFLAGS)

testParallel:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG testParallel.cpp -o testParallel $(LFLAGS)

quick_example:	quick_example.cpp
	$(CC) $(CFLAGS) -DNDEBUG quick_example.cpp -o quick_example $(LFLAGS)

testParallelNA:	testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG -DNO_AUG testParallel.cpp -o testParallelNA $(LFLAGS)

unit_tests:	unit_tests.cpp
	$(CC) $(CFLAGS) unit_tests.cpp -o unit_tests $(LFLAGS)

clean:
	rm -f testParallel testParallelNA unit_tests
