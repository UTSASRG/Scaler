# Scaler: Holistically Pinpoint Scalability Bottleneck In Whole System Stack
A tool to identify scalability issues in C/C++ programs. Then we will consider to extend to identify issues for machine learning programs. 

About Scaler, we may verify it on Firefox application (with multiple tabs to store, see Shan Lu's paper-2012), or other applications used in wPerf. 

We may also use the interception to detect tail performance issues. For instance, we could dump the specific data flow information into the file if the latency is over the average. Then we may find some commonalities between these abnormal ones. We could try to utilize such mechanism to identify the reason of server applications. We will only require uses to provide some input and output function, and then we will try to identify the reason of tail latency. 
Some related studies:
https://www.weave.works/blog/the-long-tail-tools-to-investigate-high-long-tail-latency/

What type of contributions: 

1. Identifying the data flow between components could actually identify the bottleneck. 

Two examples: 
1. We could quickly diagnose the tail latency issues. For instance, we could quickly diagnose the memory issue inside Hoard on Swaptions. 
   Or Rocksdb as the submission of HPDC submission: Memory at Your Service: Fast Memory Allocation for
Latency-critical Services

2. Providing a full-stack diagnosis. 
An example will be "firefox's multi-tab issue", or the synchronization issue of Jin Guoliang's PLDI paper. 
