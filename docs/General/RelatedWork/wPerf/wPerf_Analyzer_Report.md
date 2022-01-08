John Diep

5/26/21

wPerf Analyzer Report

What we care about:

How wPerf&#39;s analyzer constructs its wait-for graph and how it detects bottlenecks.

wPerf has a very simple solution to constructing it&#39;s wait-for graph. The wait-for graph is a directed graph, so they make use of a python library called networkX which has functions to build a directed graph based on various parameters.

To detect Strongly Connected Components (SCC), networkX also has a built-in function to perform the SCC algorithm

Then the next primary block of code is a very simple command line which allows the user to request certain functions that allow them to request work to be done on the resulting graph or get different information.

Most of the commands simply allow the user to request to view the subgraph a specific SCC represents. This then proceeds to show the edges and the weights of the sub-graph.

Another command allows the user to view all edges and information about the edges in the graph.

There is another command that lets the user view all of the threads that were recorded and the percentage of time each thread spent in a state like Running State or Runnable state or waiting or using hardware interrupts. Then it shows the total time period used to calculate the percentages.

Then there are commands that let users view function stacks of the recorded threads. There are 3 commands in total that do this, but I cannot confirm what differentiates each command from each other. There are slight differences, but nothing definitive.

Then there is a command to view a specific edge, but I cannot confirm what it does in detail because I could not get it to run.

Then there are the functions that allow the user to do work on the graph.

There are 2 commands that allow the user to request a specific SCC to view and then output into a csv file which can then be uploaded to the online graph explorer to be visualized. I cannot confirm what differentiates between the two commands as when I ran them on my useless data, they both did the same thing.

There are two commands that let the user change the view of the graph so that they may focus more closely on a specific SCC and if they want to move back to a previous view they can do so.
