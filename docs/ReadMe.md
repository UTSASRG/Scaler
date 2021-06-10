Todo:
1. How to present the percentage for application with thousands of threads.
2. How to identify abnormal thread.
3. How to combine callstack. If call the same API.
4. Perf's data doesn't consider thread contention. Thread imbalance problem.
5. Categorize thread types, group similar threads together.
6. If we have thousands of threads that belongs to multiple types. How could we present thread imbalance problem based on thread type. How to present the waiting time and normal execution time if there's synchornization inside.
7. How to present thousands of function calls. 

# Ideas


1. Perf is sampling, so some thred sync will be missed
2. Perf cannot collect accurate function duration. So it cannot understand how long synchronization takes (eg: lock contention)
3. Sage paper. (Unsupervised ML to identify performance bottleneck)
