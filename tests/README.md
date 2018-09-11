# General tests with range sum

Using

```
make
```

will give two executable files aug_sum (augmented version) and aug_sumNA (non-augmented version). This is done by setting flag -DNO_AUG in compiling time.

Using 

```
./run_aug_sum.sh -l
```

will run all experiments as shown in Table 3 in [1] on augmented sum. If you do not want to run on large input, remove "-l".

Using 

```
./runall [-r rounds] [-p threads]
```

will run all functions as shown in Table 3 in [1] with 'rounds' rounds and 'threads' threads. By default rounds=3 and threads=`nproc --all` (maximum number of threads on the machine).

Both scripts will output to both stdout and a file res.txt. The script run_aug_sum.sh will then call a python code to give all results (timings and speedups) in a file data.txt.

If user wants to directly run our executable file (aug_sum or aug_sumNA), the arguments are listed as follows:

```
./aug_sum [-n size1] [-m size2] [-r rounds] [-p] <testid>
```

The algorithms are introduced in our paper [1,2].

# References

[1] Yihan Sun, Daniel Ferizovic, and Guy E. Blelloch. Parallel Ordered Sets Using Just Join. SPAA 2016. 

[2] Yihan Sun, Daniel Ferizovic, and Guy E. Blelloch. PAM: Parallel Augmented Maps. PPoPP 2018. 