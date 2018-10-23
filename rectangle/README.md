# 2D Rectangle Query

This application deals with 2D rectangle stabbing query: given a set of n rectangles on two dimensions, and a query point, return the count (counting query) or the full list (report-all queries) of all input rectangles that contain the query point. Our implementation include two tree structures and two sweepline algorithms, making use of PAM. The algorithms are introduced in our paper [1].

rec_test.cpp + rectangle.h:
A tree structure (RecTree in [1]) answering report-all queries.

rec_sweep.cpp + rec_sweep.h:
A sweepline algorithm (RecSwp in [1]) answering report-all queries.

rec_test2.cpp + rectangle2.h:
A tree structure (RecTree* in [1]) answering counting queries.

rec_sweep2.cpp + seg_sweep2.h:
A sweepline structure (RecSwp* in [1]) answering counting queries.



Using 

```
make
```

will give the all executable file: seg_test (SegTree), seg_sweep (SegSwp), seg_test_star (SegTree*) and seg_sweep_star (SegSwp*).

To directly run the executable files, one can try:

```
./rec_sweep  <n> <rounds> <num_queries> <dist> <win>
./rec_test  <n> <rounds> <num_queries> <dist> <win> <rsv>
./rec_test2  <n> <rounds> <num_queries>
./rec_sweep2  <n> <rounds> <num_queries>
```

where 'n' stands for the number of rectangles, 'rounds' is the number of rounds, 'num_queries' is the number of queries, dist=0 means random rectangle size, dist=1 means to use 'win'/2 as the average edge length of input rectangles, rsv is the estimated size of output list per query. 

# References

[1] Yihan Sun and Guy Blelloch. Parallel Range, Segment and Rectangle Queries with Augmented Maps. ALENEX 2019.
