**Type of Data gathered:**



*   Independent Variables: Line chosen for virtual speedup and the amount of virtual speedup
*   Dependent Variable: Rate of visits to each progress point
    *   Total # of visits to each progress point 
    *   Effective Duration of the experiment (real runtime - total length of all pauses)

**How data is aggregated:**



*   Can combine performance experiments with same independent variables by summing up progress point visits and experiment durations 
*   Groups experiments by line that was virtually sped up
    *   Requires a measurement with 0% virtual speed up
        *   Throws out experiments without a 0% speed up measurement
*   Discards lines with fewer than 5 different virtual speed up amounts
*   Computes percent program speedup for each grouped experiment: % change in rate visits to each progress point over baseline

**Understanding Data:**



*   Sorts graphs by slope of their linear regression to help users identify important lines
    *   Steep upward slopes (overall positive slope)
        *   Indicates positive optimization possibility
    *   Flat line 
        *   Indicates no optimization possible
    *   Steep downward slope (overall negative slope)
        *   Indicates optimization may lead to worse performance
        *   This may be an indication of contention, optimization messes up critical path

**Implementation:**



*   By default, “COZ will only test potential optimizations in the main program’s source files”
*   Main program threads will sample their instruction pointers and map them to source lines
*   When a virtual speedup occurs, we can calculate the effect on the original program’s performance via the formula: 1- p<sub>s</sub> / p<sub>0</sub>. 
    *   P<sub>s</sub> indicates the period between progress point visits with a virtual speedup s
    *   P<sub>0</sub> is the baseline period between progress point visits where no virtual speedup is added (thus it is just normal execution)
    *   1 Indicates the original program’s natural performance and typically p<sub>s</sub>  is larger than p<sub>0</sub> as p<sub>s</sub> is measured after Coz adds a delay thus increasing execution time. This then means that we typically expect the effect on performance to be negative, which makes sense since adding execution time will obviously worsen performance.
*   Does not perform a gradual systematic increase in virtual speedups until no effect is observed on program performance
    *   Reason for this: This may overstate or understate performance depending on when and where the performance experiments are performed
        *   The WHEN and WHERE: 
            *   It is often the case that there is a warm-up period for programs, thus if we perform this systematic experiment during this warm-up period, we may observe that lines don’t have as much impact on performance, but that may just be due to the fact that they do not have a significant role at that point in time, but will have one later on. Thus by performing the experiment now, this means we may undervalue a line due to not measuring at the later point in time. 
            *   We can also overvalue a line by measuring it at a point in time where it has a significant impact on performance but does not later on in the execution.
            *   Thus the authors try to solve this problem by doing random experiments to attempt to record at different points in time for lines as to better get a sense of the impact of the lines
    *   In general, systematic approach to virtual speedups may lead to systematic bias to output data
*   When performance experiment completes, profiler thread logs:
    *   Current time
    *   \# of delays inserted for virtual speedup
    *   Size of delays inserted for virtual speedup
    *   Running count of samples in the selected line
    *   Values of all progress point counters

**Progress points**



*   Supports Source-level, breakpoint, and sampled progress points
    *   Source-Level: Requires program modification, Coz allows for a macro to be inserted which counts as a progress point
    *   Breakpoint: Specified at command-line, uses perf_events to establish breakpoints
    *   Sampled: Specified at command-line
        *   Caveat: Does not keep count of number of visits to progress point, keeps count of # of samples made at specified line
*   Uses Little’s Law: L = lambda * W to measure latency
    *   L = # of requests in progress
    *   lambda = arrival rate
    *   W = average latency
    *   Can rewrite to allow for calculating average latency = L / lambda
    *   Little’s Law works only for stable systems => Meaning Arrival rate does not exceed service rate
        *   Not a problem as profilers are used to optimize systems that work, systems that work are usable which are inherently stable. 
            *   There cannot exist a usable system that is unstable because an unstable system would mean that the system cannot handle the requests arriving because they are arriving too fast, thus it would be unusable

**Virtual Speedup**



*   “If f has an average runtime of  tf each time it is called and threads are paused for time d each time f runs, then f has an effective average runtime of  t<sub>f</sub> - d.”
*   “The pauses inserted for virtual speedup increase the total runtime by nf *d, where nf is the total number of times f by any thread. Subtracting nf*d from the total runtime with virtual speedup gives us the execution time we would measure if f had runtime tf - d.”
*   “COZ periodically samples the program counter and counts samples that fall in the line selected for virtual speedup. Then, other threads are delayed proportionally to the number of samples.”

**Sample Attribution**



*   If a chain of calls are sampled then the source line that led to the chain of calls will have those samples aggregated/attributed to the source line
    *   “For example, a program may call printf, which calls vfprintf, which in turn calls strlen. Any samples collected during this chain of calls will be attributed to the source line that issues the original printf call.”



**Coz Usable Ideas**



*   Progress points are pretty much already implemented by scaler, start and end timestamp count as progress point for an external function
    *   Thus latency in the context of Coz is the same as duration in Scaler
    *   Throughput can be calculated by calculating the rate of visits to a progress point
        *   Take total # of visits to end progress point of function and divide by the total accumulated execution time of function. This will represent the average throughput for the function. (Does not take into account synchronization)
*   The idea behind a virtual speedup is that when we delay a function by some amount then the resulting effect on the overall execution time which is described as the n<sub>f</sub> * d which is the total # of times f was run multiplied by the delay applied is how much we may see as an optimization on the same program. 
    *   This seems easy to implement at glance. (Needs more consideration)
    *   Coz uses a sampling approach to implement virtual speedup
        *   Periodically samples program counter and maintains count of samples that happened on the selected line for virtual speedup
            *   Uses delta t = d / P to determine the value d to delay program by
            *   P is sampling period
            *   D is the delay we apply
            *   Delta t represents the percent change in runtime, thus to cause a 25% virtual speed up, we would plug in ¼ to Delta t and find out that we need to insert a delay that is ¼ of the sampling period. 
            *   This also means that we don’t need to know the original runtime of the selected line during recording, as long as we have a defined sampling rate, then we can know how much we need to delay the program by