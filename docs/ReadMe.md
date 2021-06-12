

[toc]

# Data aggregation

### How to present the percentage for application with thousands of threads?

- Thought 1

Categorize thread types, group similar threads together

We may group similar threads by callstack and compare the overall latency of that callstack. If we find edge case, we report it. It should be the tail latency problem?

- Thought 2

If we call the same API, we could combine callstack.  

- Thought 3

If we can retrieve signature of thread creation function (e.g. pthread_create()), maybe we can categorize threads based on their thread function. 
Then to know the thread function, we would have to somehow retrieve that from the signature (Maybe something like a function id or address). 
We don't have to know what the function is in specific, but perhaps we simply need to know that multiple threads are calling the same thread function. 

# Finding bottleneck

### How to identify abnormal threads (Thread imbalance) if we have thousands of threads?

- Thought 1

Maybe we could merge all thread's call stack and latency information into a single graph. Find subgraphs that overlaps with most callstacks. And find thread who has significantly more execution time than its counterparts.

### Use machine learning to find performance bottleneck.

- Thought 1 

Use unsupervised machine learning. Check Sage paper. (Steven will send out an email after he finalize his notes.)

### Use machine learning to find tail latency

- Thought 1

Check related work for details.



# Better visualization

### Is pie-chart the optimal solution?

A problem with pie chart it that it merges detailed information. For tail latency problem, an averaged thread will submerge in its peers. This cannot be found from pie chart.

- Thought 1

A pie chart in the following can show more information on a graph. 

<img src="imgs/ReadMe/image-20210610182556355.png" alt="image-20210610182556355" style="zoom:33%;" />

<img src="imgs/ReadMe/image-20210610182608989.png" alt="image-20210610182608989" style="zoom:33%;" />



- Thought 2 

Flame graph can show detailed call path. But it also hides threads with tail latency problem.

We may also have flame graph?

- Thought 3

Pie charts can only show the symptom (some component is slow) rather than the root cause (why that component is slow). 

Maybe a slow component itself isn't problem, and the slow component is caused by other components which account for only a small fraction of time.

But we need a good example other than thread synchronization to convince people this is true.

### How to present the waiting time and normal execution time if there's synchronization inside.

- Thought 1

We could use a sub-section inside a bar-chart section to show the synchronization overhead. But we cannot show the threading wait-for hierarchy in this way.

### How to present thousands of function calls? 

- Thought 1

This may not be a big issue for scaler, since scaler doesn't care about the applications, but only library functions. For scaler, if there are many library calls, maybe we could integrate the library name.

### Show the system call type invoked by library functions.

- Thought 1

Originally, library functions are black-boxes. We could manually categorize system call. And report the corresponding type and amount of system call a library function is calling. 

Thus, we could give user hints about what the library function is doing. And thus, users can compare different API implementations.

# Other issues

### Check perf's data format to see whether perf's cycles represent function duration.

```
libScalerHook-d 82415  7488.768824:         97 cycles: 
        ffffffffaf476316 native_write_msr+0x6 ([kernel.kallsyms])
        ffffffffaf408328 x86_pmu_enable+0x118 ([kernel.kallsyms])
        ffffffffaf6136fd perf_pmu_enable.part.0+0xd ([kernel.kallsyms])
        ffffffffaf61ae27 __perf_event_task_sched_in+0x1a7 ([kernel.kallsyms])
        ffffffffaf4cf36d finish_task_switch+0xfd ([kernel.kallsyms])
        ffffffffaf4d783f schedule_tail+0xf ([kernel.kallsyms])
        ffffffffaf404468 ret_from_fork+0x8 ([kernel.kallsyms])
            7f21e2d47285 __GI___clone+0x35 (inlined)

libScalerHook-d 82415  7488.768829:       2489 cycles: 
        ffffffffaf476316 native_write_msr+0x6 ([kernel.kallsyms])
        ffffffffaf408328 x86_pmu_enable+0x118 ([kernel.kallsyms])
        ffffffffaf6136fd perf_pmu_enable.part.0+0xd ([kernel.kallsyms])
        ffffffffaf61ae27 __perf_event_task_sched_in+0x1a7 ([kernel.kallsyms])
        ffffffffaf4cf36d finish_task_switch+0xfd ([kernel.kallsyms])
        ffffffffaf4d783f schedule_tail+0xf ([kernel.kallsyms])
        ffffffffaf404468 ret_from_fork+0x8 ([kernel.kallsyms])
            7f21e2d47285 __GI___clone+0x35 (inlined)
```

John thinks perf's cycle only report the longest duration in each sampling event, so there's no way to know how long each sub function actually executes. While steven thinks perf will report accurate sample count and just create a new output when it detects there's changes to call stack.

For example, john thinks in this example the output of perf can only tell us  **native_write_msr+0x6** took 97 cycles + 2489 cycles. **x86_pmu_enable+0x118** may take fewer cycles, but perf doesn't record it. While steven thinks the previous output means the first part of stack trace doesn't change for 97 cycles, and then switched to the stack trace on the bottom. So it can tell us  **native_write_msr+0x6** took 97 cycles + 2489 cycles, **x86_pmu_enable+0x118** also took  97 cycles + 2489 cycles.



# Parameter Analysis

- Thought 1

For some known APIs eg: pthread library, system call. We already know their signature, so we can parse the parameters and do some analysis on them. Eg: Use them as ML features to predict latency .etc. 

# Casual profiling

- Thought 1

Like coz,, we can also delay function calls in hook functions and re-run the application to calculate potential speedup. (Steven will check if this has the potential to work)

# Overcome stack protection

- Thought 1

Make libScalerHook work with libraries that compiles with stack protection. (Need to check how other tools bypass such limit. Maybe it's doable maybe it's not. For libraries that enables Intel CET, I think libScalerhook won't work at all. But intel CET seems to be disabled by default, so we may only need to worry about stack pointer protection.)

# Debugging symbol sideload

- Thought 1

We could load debugging symbols seperately with libScalerhook, so we don't require debugging symbol in target application, but we still have deubgging information.


# Code injection

- Thought 1

Inject code into process rather than link it to reduce dependency requirement.

# Dynamic profiling

- Thought 1
Untinstall hook for functions that doesn't change much.
