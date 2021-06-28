# Lib-Analyzer
### Using with Perf
1. Run ```perf record``` with the ```--call-graph dwarf``` option on your application.
   
   Can specify an output file with the ```-o [output file]``` option.

   Can specify target sampling frequency with ```--freq=[frequency value]```. Default frequency is 1 kHz.

2. Run ```perf script``` with ```--i [input file]``` and redirect that output into an output file.

   If input file is not used, then perf script will try to find a perf.data file that perf record creates by default with an output file is not specifed.

   Thus if output file is specified, use the ```--i [input file]``` option.

3. If only using samples, then one can use the stackcollapse-perf.pl perl script to convert the perf script output file into a semicolon seperated format that my Data Aggregator script will accept. 
   
   Note: I edited the stackcollapse-perf.pl script from the original version in Brandon Gregg's Flame Graph repo to always report library/shared object names. Just using the original code would result in the loss of library names and mess up the data aggregator.
4. After this, go to the main Lib-analyzer instructions

### Running Lib-Analyzer
1. Run DataAggregator.py on your data, if it succeeds, then a json file should have been created with aggregated forms of your data. The data structure stored in the json file is explained in detail in DataAggregator.py.
   
   Note: Currently assumes a semicolon seperated format, explained in detail in the code itself.
  
   #### Have not yet implemented:

   - Easy way to define a file path for input file and output files
   - May need to create a seperate script to convert data into the semicolon seperated format that DataAggregator.py needs
   - Need to determine whether to allow user to choose whether to use samples or timing information in their data.
   
2. Run Charter.py with the json file. This script will generate a pie chart based on the data in the json file.

