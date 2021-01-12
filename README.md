# PAM

PAM (Parallel Augmented Maps) is a parallel C++ library implementing the interface for sequence, ordered sets, ordered maps and augmented maps [2]. It uses the underlying balanced binary tree structure using join-based algorithms [1,2]. PAM is highly-parallelized, safe for concurrency, theoretically work-efficient, and supporting efficient GC. PAM supports four balancing schemes including AVL trees, red-black trees, treaps and weight-balanced trees (default).

PAM also implements another implementation of augmented maps, which is the prefix structure [3]. It is used for parallel sweepline algorithms for computational geometry problems.

This repository also contains examples of using PAM for a variety of applications, including range sum, 1D stabbing queries, 2D range queries, 2D segment queries, 2D rectangle queries, inverted index searching, implementing an HTAP database benchmark combining TPC-H  queries and TPC-C transactions [5], MVCC transactional system with precise GC, etc. Detailed descriptions of the algorithms and applications can be found in our previous papers [1,2,3,4] and the tutorial in PPoPP 2019 [6].

PAM uses the PBBS library (https://github.com/cmuparlay/pbbslib) as subroutines for parallel operations, including sorting, scan, the scheduler, etc. To access the PAM libaray, you can use:

```
git clone git@github.com:cmuparlay/PAM.git
cd PAM/c++/pbbslib
git submodule init
git submodule update
```

Or more simply, use:

```
git clone --recurse-submodules git@github.com:cmuparlay/PAM.git 
```

## Interface:

To define an augmented map using PAM, user need to specify the parameters including type names and (static) functions in the entry structure ``entry''.

* typename key_t: the key type (K),
* function comp: K x K -> bool: the comparison function on K (<_K)
* typename val_t: the value type (V),
* typename aug_t: the augmented value type (A),
* function from_entry: K x V -> A: the base function (g)
* function combine: A x A -> A: the combine function (f)
* function get_empty: empty -> A: the identity of f (I)

Then an augmented map is defined with C++ template as 

```
augmented_map<entry>.
```

Here is an example of defining an augmented map "m" that has integer keys and values and is augmented with value sums (similar as the augmented sum example in our paper [1]):

```
struct entry {
  using key_t = int;
  using val_t = int;
  using aug_t = int;
  static bool comp(key_t a, key_t b) { 
    return a < b;}
  static aug_t get_empty() { return 0;}
  static aug_t from_entry(key_t k, val_t v) { 
    return v;}
  static aug_t combine(aug_t a, aug_t b) { 
    return a+b;}
};
augmented_map<entry> m;
```

Note that a plain ordered map is defined as an augmented map with no augmentation (i.e., it only has K, <_K and V in its entry) and a plain ordered set is similarly defined as an augmented map with no augmentation and no value type.

```
pam_map<entry>
pam_set<entry>
```

To declare a simple key-value map, here's a quick example:

```
struct entry {
  using key_t = int;
  using val_t = int;
  static bool comp(key_t a, key_t b) { 
    return a < b;}
};
pam_map<entry> m;
```

Another example can be found in [2], which shows how to implement an interval tree using the PAM interface.

Keys, values and augmented values and be of any arbitrary types, even an another augmented_map (tree) structure. 

## Hardware dependencies

Any modern (2010+) x86-based multicore machines.  Relies on 128-bit CMPXCHG (requires -mcx16 compiler flag) but does not need hardware transactional memory (TSX).  Most examples given in our scripts require 64GB memory, but range_query requires 256GB memory and aug_sum on the large input requires 1TB memory.  All the examples can take smaller input size by setting command line arguments.

## Software dependencies
PAM requires g++ 5.4.0 or later versions.  The default scheduler is the PBBS scheduler, but it also supports using Cilk Plus extensions or OpenMP (and of course just sequentially using g++).  You can specify one by using -DCILK, -DOPENMP or -DHOMEGROWN. 

The scripts that we provide in the repository use "numactl" for better performance. All tests can also run directly without "numactl".

We use python to write scripts to organize all results and compute speedup. It is not required to run tests.

## Applications:
In the sub-directories, we show code for a number of applications. They are all concise based on PAM, most of which need about 100 lines of code each.  All tests include parallel and sequential running times.  The sequential versions are the algorithms running directly on one thread, and the parallel versions use all threads on the machine using "numactl -i all".

In each of the directories there is a separated readme, makefile and a script to run the timings for the corresponding application.

We recommend to use numactl -i all on all parallel tests.


### General tests and range sum 
In directory tests/. More details about the algorithms and results can be found in our paper [2]. The tests include commonly-used functions on sequences, ordered sets, ordered maps and augmented maps.

### 1D stabbing query 
In directory interval/. More details about the algorithm and results can be found in our paper [2]. Our version implements a interval tree structure for stabbing queries.

### 2D range query
In directory range_query/. More details about the algorithm and results can be found in our paper [2,3]. We implemented both a range tree structure and a sweepline algorithm for 2D range queries.

### 2D segment query
In directory segment/. More details about the algorithm and results can be found in our paper [3]. We implemented both a segment tree structure and a sweepline algorithm for 2D segment queries.

### 2D rectangle query
In directory rectangle/. More details about the algorithm and results can be found in our paper [2,3]. We implemented both a tree structure and a sweepline algorithm for 2D rectangle queries (in directory common/).

### Inverted index searching
In directory index/. More details about the algorithm and results can be found in our paper [2]. Our code builds an inverted index of a dataset of wikipedia documents.

### Version maintenance for transactional memories
To be updated. More details about the algorithm and results can be found in our paper [4]. 

### Implementing an HTAP database system: TPC benchmark testing [5]
In directory tpch/. Our code builds indexes for TPC-H workload, which supports all 22 TPC-H analytical queries, and 3 types of TPC-C-style update transactions. 

## Reference
[1] Guy E. Blelloch, Daniel Ferizovic, and Yihan Sun. Parallel Ordered Sets Using Just Join. SPAA 2016 (also, arXiv:arXiv:1602.02120. 

[2] Yihan Sun, Daniel Ferizovic, and Guy E. Blelloch. PAM: Parallel Augmented Maps. PPoPP 2018. 

[3] Yihan Sun and Guy Blelloch. Parallel Range, Segment and Rectangle Queries with Augmented Maps. ALENEX 2019 (also, arXiv:1803.08621).

[4] Naama Ben-David, Guy E. Blelloch, Yihan Sun and Yuanhao Wei. Multiversion Concurrency with Bounded Delay and Precise Garbage Collection. SPAA 2019 (also, arXiv:1803.08617).

[5] TPC benchmarks. http://www.tpc.org/

[6] Yihan Sun and Guy Blelloch. 2019.  Implementing parallel and concurrent tree structures. Tutorial in PPoPP 2019.
