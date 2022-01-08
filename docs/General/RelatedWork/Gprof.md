

# Mechanism

Gprof works by **automatically instrumenting** your code during compilation, and then sampling the application's program counter during execution. 

Gprof uses **sampling based approach**. Sampling data is saved in a file, typically named gmon.out, which can then be read by the **gprof** command. Gprof was developed as an extension of the prof command, by providing a call graph of the program's execution.

After getting sampling data, gprof attribute child function execution time into parent function execution time. eg:

If A calls B for 20 times, C calls B 80 times, and B takes 10ms to run. Then 2ms is added to A, 8ms is added to C.

Compared to **Prof**, Gprof attributes child function to parent function.

# Requirement

Add -pg to compilation flag


# Reports

## Flat Profile

The Flat Profile section shows the total amount of time your program  spent executing each function, sorted by decreasing run-time.

```
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
time   seconds   seconds    calls   s/call   s/call  name
28.25      0.50     0.50     1031     0.00     0.00  advanceParticleGroup
11.30      0.70     0.20     1722     0.00     0.00  scatterParticleGroup
  7.34      0.83     0.13  1235596     0.00     0.00  contributeCurrent
  5.65      0.93     0.10     4127     0.00     0.00  indexSort
  3.67      1.00     0.07  2950376     0.00     0.00  getEcenterField
  3.39      1.06     0.06  9638242     0.00     0.00  getCell
  2.26      1.10     0.04   896527     0.00     0.00  matrixXmatrix
  1.98      1.13     0.04  3572325     0.00     0.00  getEcenter
...
[text deleted]
...
  0.00      1.77     0.00        1     0.00     0.00  setSurfaceCells
  0.00      1.77     0.00        1     0.00     0.00  setTemporalParameters
  0.00      1.77     0.00        1     0.00     0.00  setTimeStep
  0.00      1.77     0.00        1     0.00     0.00  setVeryThinConductingObjects
  0.00      1.77     0.00        1     0.00     0.00  setupMediumModels
  0.00      1.77     0.00        1     0.00     1.68  timeloop

%         the percentage of the total running time of the
time       program used by this function.

cumulative a running sum of the number of seconds accounted
seconds   for by this function and those listed above it.

self      the number of seconds accounted for by this
seconds    function alone.  This is the major sort for this
           listing.

calls      the number of times this function was invoked, if
           this function is profiled, else blank.

self      the average number of milliseconds spent in this
ms/call    function per call, if this function is profiled,
           else blank.

total     the average number of milliseconds spent in this
ms/call    function and its descendents per call, if this
           function is profiled, else blank.

name       the name of the function.  This is the minor sort
           for this listing. The index shows the location of
           the function in the gprof listing. If the index is
           in parenthesis it shows where it would appear in
           the gprof listing if it were to be printed.
```

## Call graph

```
granularity: each sample hit covers 2 byte(s) for 0.03% of 28.91 seconds
index % time    self  children    called     name
                0.00   16.73       1/1           .__scalbn [2]
[1]     57.9    0.00   16.73       1         .sphot [1]
               12.79    3.80       1/1           .execute [3]
                0.11    0.00       1/1           .copypriv [14]
                0.01    0.00       1/1           .genmesh [34]
                0.01    0.00       1/1           .copyglob [33]
                0.00    0.00       1/1           .seedranf [44]
                0.00    0.00       1/6184794     .ranf [5]
                0.00    0.00      10/13          .second [49]
                0.00    0.00       1/1           .allocdyn [51]
                0.00    0.00       1/1           .rdinput [57]
                0.00    0.00       1/1           .genxsec [52]
                0.00    0.00       1/1           .writeout [59]
-----------------------------------------------
                                                
[2]     57.9    0.00   16.73                 .__scalbn [2]
                0.00   16.73       1/1           .sphot [1]
-----------------------------------------------
               12.79    3.80       1/1           .sphot [1]
[3]     57.4   12.79    3.80       1         .execute [3]
                0.24    3.55 6184792/6184794     .ranf [5]
                0.02    0.00      24/24          .plnkut [24]
                0.00    0.00       1/1           .copyseed [46]
...
[text deleted]
...
-----------------------------------------------
                0.00    0.00       1/1           .sphot [1]
[57]     0.0    0.00    0.00       1         .rdinput [57]
                0.00    0.00       1/1           .rdopac [58]
-----------------------------------------------
                0.00    0.00       1/1           .rdinput [57]
[58]     0.0    0.00    0.00       1         .rdopac [58]
-----------------------------------------------
                0.00    0.00       1/1           .sphot [1]
[59]     0.0    0.00    0.00       1         .writeout [59]
                0.00    0.00      64/64          .wroutput [47]
                0.00    0.00       1/13          .second [49]
-----------------------------------------------
                0.00    0.00       1/1           .genmesh [34]
[60]     0.0    0.00    0.00       1         .zonevols [60]
-------libScalerHook-demoapps-FuncCall----------------------------------------
```

## Reorder Call graph




# Weakness

1. Sampling based, not very accurate.
2. Their attribution method assumes function takes constant time to execute. 
3. For recursive programs, Call graph has loop. The attribution procedure may continue infinitely. gprof uses strongly connected components to detect such loop and replace it with a single node to avoid this. It will display the measured time of each function in the loop. 
4. gprof can't discover inline function
5. 




# Reference

https://blog.csdn.net/ygtlovezf/article/details/77718663