

# Ideas for libAnalyser:

1. How to present the percentage for application with thousands of threads?
2. How to identify abnormal threads if we have thousands of threads?
3. How to combine callstack. If call the same API. We may also have frame graph.
4. Perf's data doesn't consider thread contention. Thread imbalance problem.
5. Categorize thread types, group similar threads together.
6. If we have thousands of threads that belongs to multiple types. How could we present thread imbalance problem based on thread type. How to present the waiting time and normal execution time if there's synchornization inside.
7. How to present thousands of function calls? However, this may not be a big issue for scaler, since scaler doesn't care about the applications, but only library functions. For scaler, if there are many library calls, maybe we could intergrate the library name.
8. Check Sage paper. (How to use unsupervised ML to identify performance bottleneck)
9. Check perf's data format to see whether perf's cycles represent function duration.
10. Pie charts can only show the symptom (some component is slow) rather than the root cause (why that component is slow). eg: 
11. Categorize system call and profile system call to give user hints about what the an library API is doing. (Originally it's a blackbox). Users can compare different API implementations.
12. Casual profiling. For Coz, we can delay function calls in hook functions. To calculate the potential speedup.
13. Predict profiling data using ML rather than do the actual profiling. (Q: Which semantic data can we collect as features with current information?)
14. For some known APIs eg: pthread library, system call. We already know their signature, so we can parse the parameters and do some analysis on them. Eg: Use them as ML features to predict latency .etc. 


# Ideas for libScalerHook:




