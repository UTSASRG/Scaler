# Scaler: Holistically Pinpoint Scalability Bottleneck In Whole System Stack
A tool to identify scalability issues in C/C++ programs. Then we will consider to extend to identify issues for machine learning programs. 

About Scaler, we may verify it on Firefox application (with multiple tabs to store), or other applications used in wPerf. 

We may also use the interception to detect tail performance issues. For instance, we could dump the specific data flow information into the file if the latency is over the average. Then we may find some commonalities between these abnormal ones. We could try to utilize such mechanism to identify the reason of server applications. 
