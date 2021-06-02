Main Code Body (Bottleneck.py):

1. The code starts by creating a table called btable which displays every recorded thread and information about how much time is spent in a specific state along with the total time period being used to calculate the portion of time.
2. While(1) Loop
  1. Try/Except Block
    1. Try:
      1. Check for if there is only one SCC left in the graph and if corresponding SCC has enough elements
        1. This causes the program to try and reduce/simplify the graph further to get more SCCs
      2. Then they have a very bare bones command line that lets the user request for specific functions
        1. Q for quit
        2. Gw [SCC ID #]
          1. Same as command g except it
        3. G [SCC ID #]
          1. This will output the edges of the requested SCC then it will write them into the waitfor.csv file which can be used with the online graph explorer provided for visualization
        4. S [SCC ID #]
          1. Displays information about the sub graph that the SCC consists of: The edges and the weights
            1. Will not display lone nodes
        5. A for display all edge information
        6. B for back
          1. Reverts a change done to the graph
        7. O
          1. Displays btable variable
            1. This will display each thread that was recorded and the portion of time they spent in a specific state
              1. Columns are:
                1. Running
                2. Runnable
                3. Wait
                4. Hardware Int Request
                5. Software Int Request
                6. Network
                7. Disk
                8. Other
                9. Unknown
                10. Total Time Period
        8. Edge [thread/process id] [thread/process id]
          1. Displays the edge between two processes/threads (?)
        9. Wt [thread/process id]
          1. Displays the function stack for a specific thread and the frequency of each function, exactly the same as fs except it also displays the function stack again above the pretty table for some reason
        10. Wk [thread/process id] [thread/process id]
          1. This does a comparison(?) of two threads&#39; function stacks
        11. Fs (Function Stack) [thread/process id]
          1. Displays the function stack for a specific thread and the frequency of each function
          2. Did not work when I tried, but most likely because my data is useless
        12. [SCC #]
          1. This requests the script to focus in on a specific SCC, like zooming in on a specific section of the graph and ignoring all other nodes outside of our view
          2. Is buggy, can lead to an infinite loop that cannot be exited unless we terminate the program
3. scc\_graph()
  1. Uses SCC algorithm to find all the SCCs in the directed wait-for graph
  2. Parses the SCCs to determine which are normal and are bottlenecks
    1. Determines SCCs are bottlenecks by checking if there are no outgoing edges from the SCC, this means the SCC is a knot or sink
  3. Also determines if SCCs are waiting for other SCCs (treats them like a vertex in another wait for graph), this is for displaying inter-waiting amongst SCCs
