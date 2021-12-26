# scaler: Holistically Pinpoint Scalability Bottleneck In Whole System Stack

## Overview

A tool to identify scalability issues in C/C++ programs. Then we will consider to extend to identify issues for machine learning programs. 

About scaler, we may verify it on Firefox application (with multiple tabs to store, see Shan Lu's paper-2012), or other applications used in wPerf. 

We may also use the interception to detect tail performance issues. For instance, we could dump the specific data flow information into the file if the latency is over the average. Then we may find some commonalities between these abnormal ones. We could try to utilize such mechanism to identify the reason of server applications. We will only require uses to provide some input and output function, and then we will try to identify the reason of tail latency. 
Some related studies:
https://www.weave.works/blog/the-long-tail-tools-to-investigate-high-long-tail-latency/

What type of contributions: 

1. Proposes to utilize the data flow between components to identify the performance bottleneck. This principled method could be utilized to find the bottleneck inside the whole software stack. 
2. Develops a detailed method of capturing data flow between components. That is, we propose to intercept PLT invocations to identify data flow among components, which requires no changes of programs and imposes medium overhead. 
3. Proposes multiple integration techniques to further identify root causes of performance bottlenecks. For instance, we could integrate information of serial and parallel phases, component-based, system call-based integration. We further design a visualized tool to visualize the performance bottleneck. 
4. We performed extensive studies on a range of applications to evaluate the effectiveness of the approach. We further utilizes two sets of applications to evaluate the performance overhead and memory overhead of the proposed approach. Our conclusion is that scaler can identify a range of performance issues while only imposing acceptable overhead.
5. We could know the series of function calls and their related information, which is impossible to know when using perf or other tools. We could support 2-phase profiling, where the first level is to identify the possible issue inside the execution, and then the second execution is to collect more fine-grained information. 

Possible shortcomings: 
1. This is not able to detect cache-related issues. Therefore, can combine with cache-related tools to consist of a full set of tools for performance analysis. 
2. It is not very easy to recognize whether there are some issues, which shares the same shortcoming with all time-based profilers, such as perf. For instance, we know that swpations runs very slow with Hoard, but how we can know whether Hoard introduces some performance issues? MemPerf could detect lots of lock contention and predict the performance issue. But Scaler could only detect that much time has been spent inside Hoard. This can provide some guidelines where to predict. 

## Exchanges with other people

We should do something (based on the communication with Yang Wang): 
- We will need to compare it with perf!
- We may try to use Coz for this case, to see how much performance improvement that we could achieve. 
- We will sell this a framework. Basically, it proposed a principle way for different types of profiling: that is, we could monitor the interaction between components to identify a series of performance issues, such as performance bottleneck, tail latency, performance variance, and freezing issue. We could evaluate different applications. 
- To test against the perf, maybe we could utilize the results of perf to approximate the results of Scaler. Generally, it takes a long time for it to approximate the results of perf. But that is not applicable for some applications that will run short. 

## Difference with sampling based tools (e.g., perf)

- We should have better idea on the processing time of different components, since it is based on more correct information. For instance, perf may only sample one access out of 1 million accesses. 
- Because of the more precise information, this allows us to perform some Coz related information to infer the component (or function) with potential performance impacts.  
- It could easily figure out some synchronization issues or IO issues that perf cannot do. For instance, if a thread is swapped out when it cannot get the lock or inside the IO, maybe perf will skip such information because the thread is scheduled out by the system. We will need to check this. 
- The perf may have the issue to handle multithreaded programs, especially if some threads are not inside the execution. Maybe we could evaluate this problem using some applications with locks, such as Hoard for swaptions or other applications.


## Development

libAnalyzer will develop under **dev-libAnalyzer**

libScalerHook will develop under **dev-libScalerHook**

Notes and documentation should be placed in **docs**, and **ReadMe.md**. These documentations will develop under **master** branch.

All unit tests are placed in ./unittests/library name/*. Preferably, all tests should use **gtest**.

**Master branch should contain the latest, working version of scaler.**

Clone this repo

```
git clone git@github.com:UTSASRG/scaler.git
```

Init submodules

```
git submodule update --init --recursive --depth=1
```

When a upstream branch updates, and you haven't pushed your updates. Always try the following command first rather than using merge directly.

```
git stash push
git rebase
git stash pop
```

**Don't** use the following command unless you clearly know the impact and is certain that others won't be affected.

```
git push -f 
```
