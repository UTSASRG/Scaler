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
   - 2nd Note: I created a separate python script called Perf_Script_Parser.py which does the same thing as stackcollapse-perf.pl
  
      - The sole difference is that given a perf script sample, we will attribute to each function in the sample's call stack the current sample's timestamp as the start timestamp and the timestamp of the next sample as the end timestamp.
      - This leads to the necessity to throw away the last sample as there is no way to attribute an end timestamp.
      - This method is not necessarily accurate in getting the execution time of the data, however we used it to get some idea of the execution time of functions recorded.
4. After this, go to the main Lib-analyzer instructions

### Running Lib-Analyzer
1. Run DataAggregator_V2.py on your data, if it succeeds, then a json file should have been created with aggregated forms of your data. The data structure stored in the json file is explained in detail in DataAggregator.py.
   
   - Note: Currently assumes a semicolon separated format, explained in detail in the code itself.
   
2. Run Charter.py with the json file. This script will generate a pie chart based on the data in the json file.
   - Needs to undergo some code clean up and refactoring to allow for more flexible charting. Currently the code needs to be directly edited to retrieve the desired data to chart form chart_data.json

### About Lib-Analyzer

#### This section will summarize Lib-Analyzer's Implementation.

- Lib-Analyzer's Goal is to take in function and timing information and analyze them to identify potential bottlenecks related to function calls, in particular, external library calls.
- Lib-Analyzer will read in call stacks and re-construct them into a tree data structure. 
  - To describe in specific, a singular branch from root to leaf will represent a unique call stack we detected from the function data.
  - Each node will contain simple aggregated data associated with a specific function and library
   
      - Information stored:
        - Function Name and Library Name (Both separate attributes)
        - Total recorded samples for the node's function
        - List of Timestamps if available, AND the user requested to use them
             - Also execution time
        - A list of child functions (These are functions that were called after the current node's function. i.e. on the next call stack level)

  - Furthermore, the depth of a node is directly correlated to call stack height. i.e. node depth of 1 => call stack height of 2 (Since node depth starts from 0 and I will assume that call stack height starts at 1)
    
#### Time Information Attribution
   
##### The various cases of timestamps

            Example tuple of timestamps:
               |---------|
              start     end
  
            GIVEN: 2 tuples of timestamps, comprised of a start and end timestamp, each associated with the
            same function
            Left to right represents forward time
  
            |-------|   |-------|
            
            Case 1: 2 timestamps tuples, but they are not overlapping
  
            
            Case 2: |-------|        
                        |--------|    
            
            2 timestamps tuples, but they are overlapping
  
            Case 3: |---------|--------|
            The timestamp tuples occur right after another
            (end timestamp of first tuple == start timestamp of second tuple)
  
            Case 4: |-----------|
                       |-----|
            Second timestamp tuple only shares time with first tuple.
            
            Case 5:    |---------|
                    |-----|
            Second timestamp tuple overlaps, but executes earlier than first timestamp tuple

##### How timestamps/execution time are handled in converter and DataAggregator_V2.

  - Caveat: We are not considering concurrency/multi-threading YET, only sequential time
  - In DataAggregator:
    - Save all of the timestamps recorded for a function in a list, sorted in ascending order based on the start timestamp
    - Perform timestamp merging if the 3rd case above occurs by simply replacing the timestamps that fit Case 3 with a single timestamp tuple of the earliest timestamp and latest timestamp
  
      - This is not limited to just a pair of timestamps we can have an arbitrary amount of timestamps that lead to Case 3. Regardless the same operation applies.
  - In Converter:
    - We perform redistribution which is simply calculating the true execution time of each function
      - The algorithm is simple:
        - Since our data structure is a generic tree, with each node representing a function and direct childs representing a function that the parent called
        - Then the execution times prior to redistribution of the direct childs represent the time that was not part of the true execution of the parent function. Therefore, we subtract the child execution times from parent's execution time which the result would then represent the true execution time of that node.
    

#### How perf samples counts are handled in converter
    
- We perform redistribution as well for the perf sample counts which in this case simply reveals the function that was at the end of a sample's call stack
  
    - We care about the function at the end of the call stack because regarding samples the end of the call stack was the executed function at the time. Furthermore, the resulting redistributed sample count represents a portion of the total recorded samples rather than counting the same sample number multiple times if we did not redistribute thus bloating our data.