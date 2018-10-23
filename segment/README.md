# 2D Segment Query

This application deals with 2D segment query: given a set of n non-crossing segments on two dimensions, and a query vertical segment, return the count (counting query) or the full list (report-all queries) of all input segments intersecting query segment. Our implementation include two tree structures and two sweepline algorithms, making use of PAM. The algorithms are introduced in our paper [1].

seg_test.cpp + segment.h:
A tree structure (SegTree in [1]) answering either counting or report-all queries for non-intersecting input segments.

seg_sweep.cpp + seg_sweep.h:
A sweepline algorithm (SegSwp in [1]) answering either counting or report-all queries for non-intersecting input segments.

seg_test_star.cpp + segment_star.h:
A tree structure (SegTree* in [1]) answering counting queries for horizontal (axis-parallel) input segments.

seg_sweep_star.cpp + seg_sweep_star.h:
A sweepline structure (SegSwp* in [1]) answering counting queries for horizontal (axis-parallel) input segments.



Using 

```
make
```

will give the all executable file: seg_test (SegTree), seg_sweep (SegSwp), seg_test_star (SegTree*) and seg_sweep_star (SegSwp*).

To directly run the executable files, one can try:

```
./seg_sweep [-n size] [-r rounds] [-q num_queries] [-d dist] [-w window] [-t querytype]
./seg_test [-n size] [-r rounds] [-q num_queries] [-d dist] [-w window] [-t querytype]
./seg_test_star <n> <min> <max> <rounds> <num_queries>
./seg_sweep_star <size> <min> <max> <rounds> <num_queries>
```

where 'size' stands for the number of points, 'rounds' is the number of rounds, 'num_queries' is the number of queries, min/max represents the min/max value of coordinates, d=0 means random query windows, d=1 means to use 'window'/2 as the average query window size (for the  y-coordinates), 'query_type' is 0 for query-all, and 1 for query-sum. 

# References

[1] Yihan Sun and Guy Blelloch. Parallel Range, Segment and Rectangle Queries with Augmented Maps. ALENEX 2019.
