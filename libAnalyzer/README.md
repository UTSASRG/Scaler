# Lib-Analyzer
### Using with Perf
1. Run ```perf record``` with the ```--call-graph dwarf``` option on your application.
   
   - Can specify an output file with the ```-o [output file]``` option.

   - Can specify target sampling frequency with ```--freq=[frequency value]```. Default frequency is 1 kHz.

2. Run ```perf script``` with ```--i [input file]``` and redirect that output into an output file.

   - If input file is not used, then perf script will try to find a perf.data file that perf record creates by default with an output file is not specifed.

   - Thus if output file is specified, use the ```--i [input file]``` option.

3. If only using samples, then one can use the stackcollapse-perf.pl perl script to convert the perf script output file into a semicolon seperated format that my Data Aggregator script will accept. 
   
   - Note: I edited the stackcollapse-perf.pl script from the original version in Brandon Gregg's Flame Graph repo to always report library/shared object names. Just using the original code would result in the loss of library names and mess up the data aggregator.
4. After this, go to the main Lib-analyzer instructions

### Running Lib-Analyzer
1. Run DataAggregator_V2.py on your data, if it succeeds, then a json file should have been created with aggregated forms of your data. The data structure stored in the json file is explained in detail in DataAggregator.py.
   
   - Note: Currently assumes a semicolon seperated format, explained in detail in the code itself.
  
   #### Have not yet implemented:
   
   - May need to create a separate script to convert data into the semicolon separated format that DataAggregator.py needs
   
2. Run Charter.py with the json file. This script will generate a pie chart based on the data in the json file.
   - NOTE: Charter.py is undergoing refactoring, it is not yet compatible with the output data structure of DataAggregator_V2.py
   
### About Lib-Analyzer

#### This section will summarize Lib-Analyzer's Implementation.

- Lib-Analyzer's Goal is to take in function and timing information and analyze them to identify potential bottlenecks related to function calls, in particular, external library calls.
- Lib-Analyzer will read in call stacks and re-construct them into a tree data structure. 
  - To describe in specific, a singular branch from root to leaf will represent a unique call stack we detected from the function data.
  - Each node will contain simple aggregated data associated with a specific function and library
   
      - Information stored:
        - Function Name and Library Name (Both seperate attributes)
        - Total recorded samples for the node's function
        - Timestamps if available, AND the user requested to use them
             - Also execution time
        - A list of child functions (These are functions that were called after the current node's function. i.e. on the next call stack level)

  - Furthermore, the depth of a node is directly correlated to call stack height. i.e. node depth of 1 => call stack height of 2 (Since node depth starts from 0 and I will assume that call stack height starts at 1)
    
#### Time Information Attribution
   
- Current idea for solution:

            ex:|---------|
              start     end
  
            |-------|   |-------|
            
            2 timestamps tuples, but they are not overlapping
            
            Current idea for solution: Store each timestamp tuple separately if they do not overlap
            AND the new timestamp tuple occurs after the current first stored timestamp tuple
            These tuples will be stored in a list in the self.timestamps attribute and it will be sorted by the
            start timestamp of each tuple in ascending order.
            Then whenever I need to update timestamps, I have to iterate through the timestamp list.
            
            The reason why I want to use a list is because I do not want to simply extend the first timestamp tuple
            with the second tuple (by swapping the end timestamp with the second end timestamp). If I do this, then
            this would result in including that break between the timestamp tuples (seen above). This break is not
            a part of the true duration of the timestamps and would bloat the true execution time of the function call.
            
            Whenever I do operations on a function's execution time, I want to be working in the realm of the original 
            execution time (from actual function start to function termination). 
            The reason why I want to do it this way is because this allows me to accurately redistribute
            execution times to obtain the true execution time of a function (The actual time spent in the function 
            and not in other functions) since child functions run during the original execution time (meaning it starts 
            after its parent) of its parent function. 
            It would not make sense to include those breaks because the child functions never run in those 
            breaks as I explained, they should run during the original execution times of the parent function
            
            or
            
            |-------|       or  |---------|--------|
                |--------|    
            
            2 timestamps tuples, but they are overlapping or they appear right after another
            
            Current idea for solution: Simply extend the first tuple with by swapping the end timestamp of the
            first tuple with the second end timestamp
            
            changing:
            |-------|
                |--------|
            to:
            |------------|
            
            If we have timestamps like this:
            
            |-----------|
               |-----|
               
            Then do nothing as the second timestamps is already covered by the first timestamps
            
                |---------|
            |-----|
            
            In this case, I declare these as impossible as my trees are stored on a thread-specific basis, so 
            function calls must occur sequentially. 
            Since these function calls are only associated with a specific thread.
- Charter.py refactoring has not been completed, thus I do not yet know how I am going to create the pie chart from the data.