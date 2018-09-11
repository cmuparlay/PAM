# Interval Tree

This application deals with 1D stabbing query: given a set of n intervals on number line, report if a given point is covered by any input intervals, or report all input intervals covering a given point.

Our implementation is an interval tree on top of PAM.

Using 

```
make
```

will give the executable file (interval).

Using

```
run_interval
```
will give the same experiment of interval trees as shown in Table 5 in [1]. 

To directly run the executable file (interval), one can try:

```
./interval n q r
```

where n stands for the number of intervals, q is the number of queries, r is the number of rounds. By default n=100000000, q=n, r=5.

# Reference
[1] Yihan Sun, Daniel Ferizovic, and Guy E. Blelloch. PAM: Parallel Augmented Maps. PPoPP 2018. 
