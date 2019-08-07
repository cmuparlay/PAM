# An HTAP database system on TPC workloads

## Introduction

The code under this directory implements an HTAP (Hybrid transactional/analytical processing) database system and tests it on TPC benckmarks. It builds indexes for the TPC-H workload. Our implementation supports all 22 pre-defined queries in TPC-H specification, and 3 types of TPC-C-style update transactions. One can run pure queries, pure updates, or a mixed mode of both, in which queries and updates are executed simultaneously. Our implementation supports snapshot isolation (SI) for Multi-version concurrency control (MVCC). In particular, updates will create new versions of the database and leave old versions for ongoing queries. Queries will see the last version committed by updates. 

## Queries and transactions

The 22 queries are implemented as in the TPC-H official documentation [1]. We adapt TPC-C-like transactions into the TPC-H workload, and especially design three transaction types as follows.

* New-Order [49%]. A customer creates a new order entry that contains x (1..7) lineitems.
* Payment [47%]. A customer makes a payment to update their account balance.
* Delivery [4%]. The company marks the lineitem records in the earliest y (1..10) un-processed orders as shipped.

The orders and lineitems are generated based on TPC-H specification. 

## Running tests

Using 

```
make
```

will give the executable file: test.

To directly run the executable files, one can try:

```
./test [-v] [-q] [-u] [-c] [-t txns] [-d directory] [-y keep_versions] [-p]
```

* -v: passing the '-v' option will enable the verbose mode to show more output information when running the tests.

* -t txns: the number of update transactions.

* -q and -u: the '-q' option means to run queries, and the '-u' option means to run updates. When neither is used, the program only loads input and exit. When only '-q' is used, all 22 queries will be executed one after another in a stream. This query stream repeats for five times. When only '-u' is enabled, the program execute a number txns of update transactions to the initial workload. When both are enabled, queries and updates are executed concurrently.

* -y keep_versions: the number of versions kept when GC is enabled.

* -c: the '-c' option decides if GC is enabled, if so, it always keep the latest keep_versions versions.

* -p: the '-p' option decides if persistence is enabled. If so, it writes the executed transactions as logs to the disk. 

* -d directory: the '-d' option specifies the path to the input file (all tables). 


The input tables in TPC-H can be generated using tpch-dbgen (https://github.com/electrum/tpch-dbgen). After getting the input tables, simply put them in the same directory and pass the directory path as the "-d" parameter. Our code outputs the geometric mean of running time across five runs for each query, as well as the geometric mean of all 22 queries. This is the performance measurement suggested by the TPC-H specification. 

We always execute one query at a time, but each query exploits full parallelism (can use all available threads, but depending on the scheduler). All updates are done sequentially (using a single thread), also one at a time.


# References

[1] TPC Benchmark(TM) H Standard Specification Revision 2.18.0, http://www.tpc.org/tpc_documents_current_versions/pdf/tpc-h_v2.18.0.pdf



