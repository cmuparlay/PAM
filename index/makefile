CC = g++ 
CFLAGS = -O2 -I../c++ -mcx16 -march=native -DCILK -fcilkplus -std=c++11 -Wall

index:	index.cpp index.h 
	$(CC) $(CFLAGS) index.cpp -o index

index_w:	index_w.cpp index_w.h 
	$(CC) $(CFLAGS) index_w.cpp -o index_w

wiki_small.txt: 
	bunzip2 wiki_small.txt.bz2

clean:
	rm -f index index_w
