# Scaler: Holistically Pinpoint Scalability Bottleneck In Whole System Stack

## Idea

A tool to identify scalability issues in C/C++ programs. Then we will consider to extend to identify issues for machine learning programs. 

About Scaler, we may verify it on Firefox application (with multiple tabs to store, see Shan Lu's paper-2012), or other applications used in wPerf. 

We may also use the interception to detect tail performance issues. For instance, we could dump the specific data flow information into the file if the latency is over the average. Then we may find some commonalities between these abnormal ones. We could try to utilize such mechanism to identify the reason of server applications. We will only require uses to provide some input and output function, and then we will try to identify the reason of tail latency. 
Some related studies:
https://www.weave.works/blog/the-long-tail-tools-to-investigate-high-long-tail-latency/

What type of contributions: 

1. Proposes to utilize the data flow between components to identify the performance bottleneck. This principled method could be utilized to find the bottleneck inside the whole software stack. 
2. Develops a detailed method of capturing data flow between components. That is, we propose to intercept PLT invocations to identify data flow among components, which requires no changes of programs and imposes medium overhead. 
3. Proposes multiple integration techniques to further identify root causes of performance bottlenecks. For instance, we could integrate information of serial and parallel phases, component-based, system call-based integration. We further design a visualized tool to visualize the performance bottleneck. 
4. We performed extensive studies on a range of applications to evaluate the effectiveness of the approach. We further utilizes two sets of applications to evaluate the performance overhead and memory overhead of the proposed approach. Our conclusion is that Scaler can identify a range of performance issues while only imposing acceptable overhead. 

Possible shortcomings: 
This is not able to detect cache-related issues. Therefore, can combine with cache-related tools to consist of a full set of tools for performance analysis. 

Two examples: 
1. We could quickly diagnose the tail latency issues. For instance, we could quickly diagnose the memory issue inside Hoard on Swaptions. 
   Or Rocksdb as the submission of HPDC submission: Memory at Your Service: Fast Memory Allocation for
Latency-critical Services

2. Providing a full-stack diagnosis. 
An example will be "firefox's multi-tab issue", or the synchronization issue of Jin Guoliang's PLDI paper. 

## Dev

libAnalyzer will develop under **dev-libAnalyzer**

libScalerHook will develop under **dev-libScalerHook**

Notes and documentation should be placed in **docs**, and **ReadMe.md**. These documentations will develop under **master** branch.

All unit tests are placed in ./unittests/library name/*. Preferably, all tests should use **gtest**.

**Master branch should contain the latest, working version of Scaler.**

## Contribution

Clone this repo

```
git clone git@github.com:UTSASRG/Scaler.git
```

Init submodules

```
git submodule update --init --recursive --depth=1
```
