```
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
```

[Doc Link](https://drive.xttech.top/s/EDF7Tw2iWdGHQcp)

Sampling:

*   Fast and efficient, but inaccurate
*   Takes samples (may be a call stack, perhaps some counter or address) at some sample rate
    *   This means it is inevitable that we miss information, some things may happen too fast or perhaps too slow that the sampler does not pick up
    *   We miss information regardless of sample rate due to the low resolution of what we gather(May detect stuff like function call stack, but we may not know every single function that was called)

Tracing

*   Slow, but accurate
*   Generally focuses on high resolution information, such as detecting every function/system call or every memory allocation
*   Has uses for performance issues that require high resolution like Tail latency because the causes of tail latency may be difficult to detect as tail latency involves events that occur very infrequently
    *   Sampling is less effective in this situation because it may miss the rare events that result in tail latency and thus we would have very little information about what causes the latency

Tail latency


*   An important issue in the modern world where fast and consistent response times matter a lot to the consumer
*   Typically involves rare events that cause bad performance
*   Has a significant impact on overall performance because 1 second out of an average of say 10 ms feels much more significant at that small of a time scale
*   Since the system expects to work at the smaller time scale, when it suddenly has to wait a significant amount of time, it gets backed up and slows down similar to throwing a wrench into some gears

From [https://danluu.com/perf-tracing/](https://danluu.com/perf-tracing/)

Google had a problem with tail latency which was could not be solved with sampling due to very low resolution of the information gathered

Thus Google had to opt for a tracing solution in order to get a much better picture of what actually occurs with their systems and was able to identify a specific thread that was causing the tail latency. They then discovered that the latency was due to a lock contention. If a sampler was used, they generally only give us information about active execution (On CPU vs OFF-CPU)

“one of the fundamental limits of a sampling profiler is how often you can sample.”