include ../Makeheader 

index:	index.cpp index.h 
	$(CC) $(CFLAGS) index.cpp -o index $(LFLAGS)

index_w:	index_w.cpp index_w.h 
	$(CC) $(CFLAGS) index_w.cpp -o index_w $(LFLAGS)

wiki_small.txt: 
	bunzip2 wiki_small.txt.bz2

clean:
	rm -f index index_w
