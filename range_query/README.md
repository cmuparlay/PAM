# 2D Range Query

This application deals with 2D range query: given a set of n points on two dimensions, and a query rectangle, return the weighted sum (query-sum) or the full list of all input points (query-all) in the query rectangle. Our implementation include a tree structure and two sweepline algorithms, making use of PAM. The algorithms are introduced in our papers [1,2].

Using 

```
make
```

will give the all executable file: rt_test (the range tree), rs_test (the sweepline algorithm for query-sum queries) and rs_test_list (the sweepline algorithm for query-all queries).

To directly run the executable files, one can try:

```
./rt_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-w window] [-t query_type]
./rs_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-w window]
./rs_test_list [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-w window]
```

where 'size' stands for the number of points, 'rmin' and 'rmax' are the upper and lower bound of coordinates, 'rounds' is the number of rounds, 'queries' is the number of queries, 'window' is the twice the average query window size (for one dimension), 'query_type' is 0 for query-all, and 1 for query-ssum. By default n=100000000, l=0, h=1000000000, r=3, q=1000, w=1000000, t=0.

# References

[1] Yihan Sun, Daniel Ferizovic, and Guy E. Blelloch. PAM: Parallel Augmented Maps. PPoPP 2018. 

[2] Yihan Sun and Guy Blelloch. Parallel Range, Segment and Rectangle Queries with Augmented Maps. ALENEX 2019.
