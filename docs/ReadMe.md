

[toc]

# Format

> The format should be.
>
> Try to use markdown editor (eg: Typora, Github Web IDE to make sure the format is correct)
>
> Use emojis to distinguish
>
> - Things that haven't been discussed with anyone :mag_right:
> - Things that discussed between steven and john but need to report to the professor: :bulb:
> - Things valuable after discussion: :heavy_check_mark:
> - Things invaluable after discussion: :x:
> - Things that have been discussed, but need further investigation :question:
>
> ```
> # Section Name
> ## Subsection Name
> 
> ### Issue_Category_Number - Category
> 
> ### Issue_Category_Number - Issue_Number Breif_Description
> 
> - :emoji: Thought 1
> Descriptions about your idea
> 	- :emoji: Comment 1
> 	Some comments regarding thought 1
> 	- :emoji: Comment 2 
> 	Some comments regarding thought 1
> - :emoji: Thought 2
> Descriptions about your idea
>     - :emoji: Comment 1
>     Some comments regarding thought 2
>     - :emoji: Comment 2
>     Some comments regarding thought 2
> ```
>
> 

> To ensure most effective communication: 
>
> 1. Always use **Topic sentence** + Details.  Set topic sentence to **bold typeface**.
> 2. Change paragraph if you are talking about things not covered by topic sentence.
> 3. Be brief is possible
> 4. Use markers. bullet points
> 5. Use figure to show what you mean



# Background

**We already collect data through tracing based method. How to present these data?**  We currently collect a tuple of 4 (timestamp, library name, thread id, function name) and the function invocation sequence. (Not call stack, but only external function invocation sequence).

**The overall goal is to provide a visualization that tells users which component they should primarily focus on.** Preferably, the tool should report relative importance of each functions. (eg: If the visualization tool shows libraryA is more important than libraryB, then it should mean fixing libraryA would get more benefits).

**To achieve this goal, we may meet challenges.** For example, a component that takes more averaged execution time may not necessarily mean it is the most worthy component to fix. 

**We should think about two types of questions**: 

1. Which factors may cause averaged time to not reflecting the most worthy component to fix? 
2. What's the potential solutions to fix these problems?

# B-Discussions after June 15th meeting

## B1-Multi-threading

### B1-1 thread synchronization can change the relative importance of a function





## B2-Data changes with time







# A-Discussions before June 15th meeting

**Before June 15th, we have two confusions.** 

 1. Tail latency is not what we should focus on. 
 2. We only need to provide some statistical data.
  3. The format of data presentation is not that important. eg: Treemap and piechat doesn't make lots of improvements.

**The followings are a record of previous discussions. Things closed related to our goal should be copied into section B**

## A1-How to present the percentage for application with thousands of threads?

- :x: Thought 1
  **Categorize thread types, group similar threads together.** We may group similar threads by callstack and compare the overall latency of that callstack. If we find edge case, we report it. It should be the tail latency problem?

  - :heavy_check_mark: Comment 1

    Tail latency problem is another project.

- :heavy_check_mark: Thought 2
  **We can retrieve signature of thread creation functions to intercept thread synchronization.** eg: pthread_create(). 

  - :heavy_check_mark: Comment 1

    **To know the thread function, we would have to somehow retrieve that from the signature** (Maybe something like a function id or address). 
    **We don't have to know what the function is in specific.** Perhaps we simply need to know that multiple threads are calling the same thread function. 

   - :x:  Comment 2

     **This approach have two potential problems.** One problem with this is: Would have to have prior knowledge of thread creation function and search for them in particular. Or somehow have a systematic way of identifying thread creation functions. (Cannot simply assume that pthread_create() is the main function used to create threads, maybe there is some other thread creation function that was used like a proprietary or self made function, unless this is not a issue.)

   - :heavy_check_mark: Comment 3

     **Comment 2 is not a problem.** We can assume pthread_create would be the creation method**.** We don't have many APIs to intercept, so it would be enough to manually find these APIs. 
     **Comment 1 is what we will do**, we need some prototypes.

## A2-How to identify abnormal threads (Thread imbalance) if we have thousands of threads?

- :x: Thought 1
  **I have an idea about how to find the bottleneck.** Maybe we could merge all thread's call stack and latency information into a single graph. Find subgraphs that overlaps with most call-stack. And find thread who has significantly more execution time than its counterparts.

  - :heavy_check_mark: Comment 1

    **This is not our goal.** Our goal is to report some statistic data rather than correlate data together. We only need to report which component is the most worthy to fix.

- :heavy_check_mark: Thought 2

  **We need to be able to measure or identify the relevance of a thread to other threads.** Relevance is talking about the similarity of a thread's task is to the tasks of other threads. For example: There may be threads designated to starting new threads whereas another thread is dedicated to performing some computation like matrix multiplication perhaps and another thread is dedicated to cleaning up memory.

  **Different thread may have different contributions.** We may see that the first thread may not have very little impact on the overall execution, but if we do not count for relevance, in our final results we may falsely report that there is load imbalance because of how much execution the second thread takes up (since matrix multiplication is most likely more resource intensive than thread creation) where in reality, the "loads" of either thread have little relation to one another and thus changing the load of one thread does not directly affect the other. This is where relevance is important, by knowing how relevant a thread is to another, we may more accurately understand how the changing of loads will affect the performance of the relevant threads.

  **How do we measure relevance?** We may do this by grouping threads by their function (perhaps the best way) as described by my example in A1, thought 2 of the data aggregation section above. If we can retrieve function parameters, we may compare inputs (This is most likely the worst method as it would be probably is not scalable when considering massive inputs) and grouping threads by similar inputs.

  - Comment 1

    **Thought2 has been moved to  section B, since they are relevant.**

- :x: Thought 3

  **We could ask developers to mark requests that they care.** Developers could marks functions and requests inside the develop machine and "teaches" profiler how to profile. And profilers use that information to find similar requests inside an application without debugging info. We then find similar requests inside a program in production.

  **In actual profiling process, we may even ignore requests that user doesn't want.** So the data will be cleaner and more accurate.

  - :heavy_check_mark: Comment 1

    **The overall goal is to report statistical data. Detailed profiling is out of the scope.** 

    **User intervention is not preferred.**

## A3 - Better visualization

- :x: Thought 1

  **Pie-chart is not the optimal solution for finding tail latency problem?** For tail latency problem, an averaged thread will submerge in its peers. This cannot be found from pie chart.

  - :heavy_check_mark: Comment 1

    **Tail latency is not our focus.**

- :bulb: Thought 2

  A pie chart in the following can show more information on a graph. 

  <img src="imgs/ReadMe/image-20210610182556355.png" alt="image-20210610182556355" style="zoom:33%;" />

  <img src="imgs/ReadMe/image-20210610182608989.png" alt="image-20210610182608989" style="zoom:33%;" />

  - Comment 1
    
    **This graph will be mentioned in section B.**
  
- :x: Thought 3

  Flame graph can show detailed call path. But it also hides threads with tail latency problem.

  - Comment 1
    **Tail latency is not our focus.** 

    **This graph will be briefly mentioned in section B.**

- :question: Thought 3

  **Pie charts can only show the symptom (some component is slow) rather than the root cause (why that component is slow).** Maybe a slow component itself isn't the problem, and the slow component is caused by other components which account for only a small fraction of time.

  But we need a good example other than thread synchronization to convince people this is true.

  - :heavy_check_mark: Comment 1

    **The goal of this tool is not finding the root cause.** We just need to report which component has the problem.

- :x: Thought 4

  **A Tree map might be a better and more compact visualization of our data.** Because it means we can represent all of the threads as a portion of the total execution where the total execution is the entire tree map. Then we can have the function and library portions representedx] as nested in their respective threads. This provides a more holistic view of the data as opposed to separate pi charts that require the user to click to view individual sections.

  ![image-20210615124128148](imgs/ReadMe/image-20210615124128148.png)

  - :heavy_check_mark: Comment 1
    **This doesn't improve pie-chat by much.** 

    **This graph will be briefly mentioned in section B.**

- :heavy_check_mark: Thought 5

  **To present synchronization cost, we could use a sub-section inside a bar-chart section.** But we cannot show the threading wait-for hierarchy in this way.

- :heavy_check_mark: Thought 6

  **There may not be thousands of threads to show?** This may not be a big issue for scaler, since scaler doesn't care about the applications, but only library functions. For scaler, if there are many library calls, maybe we could integrate the library name.

- :bulb: Thought 7

  **We could categorize and show system call information**. Library functions are black-boxes. We could manually categorize system call. And report the corresponding type and amount of system call a library function is calling. 

  Thus, we could give user hints about what the library function is doing. And thus, users can compare different API implementations.

## A4 - Other issues

### A4-1 Check perf's data format to see whether perf's cycles represent function duration.

- :question: ​Thought 1

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

**Steven and john have different opinions on what's the meaning of the cycles.** John thinks perf's cycle only report the longest duration in each sampling event, so there's no way to know how long each sub function actually executes. While steven thinks perf will report accurate sample count and just create a new output when it detects there's changes to call stack.

**Let's use an example to illustrate the difference.** For example, john thinks in this example the output of perf can only tell us  **native_write_msr+0x6** took 97 cycles + 2489 cycles. **x86_pmu_enable+0x118** may take fewer cycles, but perf doesn't record it. While steven thinks the previous output means the first part of stack trace doesn't change for 97 cycles, and then switched to the stack trace on the bottom. So it can tell us  **native_write_msr+0x6** took 97 cycles + 2489 cycles, **x86_pmu_enable+0x118** also took  97 cycles + 2489 cycles.



## A5 - Parameter Analysis

- Thought 1

For some known APIs eg: pthread library, system call. We already know their signature, so we can parse the parameters and do some analysis on them. Eg: Use them as ML features to predict latency .etc. 

## A6 - Casual profiling

- Thought 1

Like coz,, we can also delay function calls in hook functions and re-run the application to calculate potential speedup. (Steven will check if this has the potential to work)

## A7 - Overcome stack protection

- Thought 1

Make libScalerHook work with libraries that compiles with stack protection. (Need to check how other tools bypass such limit. Maybe it's doable maybe it's not. For libraries that enables Intel CET, I think libScalerhook won't work at all. But intel CET seems to be disabled by default, so we may only need to worry about stack pointer protection.)

## A8 - Debugging symbol sideload

- Thought 1

We could load debugging symbols seperately with libScalerhook, so we don't require debugging symbol in target application, but we still have deubgging information.


## A9 - Code injection

- Thought 1

Inject code into process rather than link it to reduce dependency requirement.

## A10 - Dynamic profiling

- Thought 1
Untinstall hook for functions that doesn't change much.
