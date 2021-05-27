# What can perf do?

Perf can already report figures in ReadMe.md

- Perf is able to profile external functions in both application and library.

![image-20210527104938420](imgs/Perf/image-20210527104938420.png)

- Perf is able to report kernel functions

  ![image-20210527105015373](imgs/Perf/image-20210527105015373.png)

- Perf is able to report of the percentage of each function.

![image-20210527105046824](imgs/Perf/image-20210527105046824.png)

- Perf doesn't require debugging symbol
- Perf uses kernel supported CPU performance counters, tracepoints, kprobes, and uprobes to intercept function execution. While scaler intercepts PLT table to implement hook. 