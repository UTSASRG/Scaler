
from DataAggregator_V2 import FunctionTree, printTree
import pprint
import json
import tkinter as tk
from tkinter import filedialog
import sys
'''
This converter script will convert the final output data of Data Aggregator into an easy to use format
for charter, that way I pre-process all of the data so I can immediately create a pie chart.
Currently it will generate a dictionary of sample totals for libraries called for a specific thread

Retrieving Total Thread Samples:
Sum up sample totals of all root nodes


'''

useTimestamps = False
# This takes in the dictionary stored in the json format and reconstructs the original output data structure
# The original output data structure being: {tid: {root function name: function tree}}
def loadData(tidDict):
    for key, treeDict in tidDict.items():
        for treeKey, tree in treeDict.items():
            tidDict[key][treeKey] = FunctionTree.reconstructNodefromDict(tree, useTimestamps)

# This will retrieve the leaves of the function trees stored in the final data structure
# What is obtained from the leaves are the attributes except for the child function list
# These leaf attributes are stored in a dictionary and then stored in a list
# This list is stored in a dictionary where the key is tid and the value is the leaf list
def getLeaves(tidData):
    leafList = []
    leafDict = dict.fromkeys(tidData.keys())
    for tid in leafDict.keys():
        leafDict[tid] = dict.fromkeys(tidData[tid].keys())
        for tree in tidData[tid].values():
            leafList += tree.retrieveLeaves()
        leafDict[tid] = leafList
        leafList = []
    return leafDict

# This will sum up the samples of the leaves with respect to the library the leaf called
# These totals will be stored in a dictionary where the key-value pair is library:sample total
# These totals will be stored with respect to a thread id so the final structure is
# {tid: {lib: sample total}}
def getLibSampTot(leafDict):
    libDict = dict.fromkeys(leafDict.keys())
    for tid, leafList in leafDict.items():
        libDict[tid] = {}
        for leaf in leafList:
            libDict[tid].setdefault(leaf['lib'], 0)
            libDict[tid][leaf['lib']] += leaf['sampleTotal']
        # print(tid, libDict)
    return libDict

# Retrieves the total samples recorded for each thread
# Returns a dictionary with a key-value pair of tid-total samples
def getThreadSampTot(tidData):
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if tidDict[tid] != None:
                tidDict[tid] += tree.sampleTotal
            else:
                tidDict[tid] = tree.sampleTotal
    return tidDict

# Retrieves the execution times of the root functions
# If retrieved before redistribution, these execution times do not represent total On-CPU execution time
# This is because these root functions may include some off-cpu time in its total execution time
# This makes sense since there may be some I/O operations and the function has to wait or something like lock contention
def getThreadRootExe(tidData):
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if tidDict[tid] != None:
                tidDict[tid] += tree.execTime
            else:
                tidDict[tid] = tree.execTime
    return tidDict

# Will retrieve each thread's total execution time
# The total execution time of the thread is determined by retrieving the earliest timestamp from all of the roots
# And retrieving the latest timestamp from all of the roots
# TODO During redistribution, I do not update timestamps only execution time, I currently use the old timestamps
# to calculate thread execution time
def getThreadTotalExecution(tidData):
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            # print(tree.timestamps)
            # If there is nothing in the dictionary, then set the current tree's timestamps as the current thread's timestamp
            if tidDict[tid] == None:
                tidDict[tid] = (tree.timestamps[0][0], tree.timestamps[-1][1])
            # If the current root's timestamp starts earlier than the current thread timestamp
            elif tree.timestamps[0][0] < tidDict[tid][0]:
                # If the start timestamp is earlier and the end timestamp is later, then set the thread's timestamp
                # as the current root's timestamp
                if tree.timestamps[-1][1] > tidDict[tid][1]:
                    tidDict[tid] = (tree.timestamps[0][0], tree.timestamps[-1][1])
                # If the current root's start timestamp is earlier, but the end timestamp is earlier or the same
                # as the thread's end timestamp
                # Then simply replace the thread's start timestamp
                else:
                    tidDict[tid] = (tree.timestamps[0][0], tidDict[tid][1])
            # If the current root's start timestamp is later or the same as the thread's start timestamp
            # But the root's end timestamp is later, then replace the end timestamp of the thread
            elif tree.timestamps[-1][1] > tidDict[tid][1]:
                tidDict[tid] = (tidDict[tid][0], tree.timestamps[-1][1])
    # After we get the timestamps, calculate the execution times
    for tid, time in tidDict.items():
        tidDict[tid] = time[1]-time[0]
    return tidDict

# This function updates that timestamp list in the update exec time function
# by adding the timestamp if no overlap occurs to the list
# or by extending timestamps when overlaps occur
def updateTimestampList(childTimeTup, childTimeList):
    index = 0
    for timestampTup in childTimeList:
        if childTimeTup[0] < timestampTup[0]:
            #   |----|
            # |---------|
            # In this case, we see that the new timestamp tuple encompasses the old one, thus we replace the old one
            if childTimeTup[1] > timestampTup[1]:
                childTimeList[index] = childTimeTup
                return
            #   |---|      |----|         |-------|
            # |-----| or |----|    or  |--|
            # In this case, we simply extend the old tuple to the left with the new tuple
            elif childTimeTup[1] >= timestampTup[0]:
                # print(f"What, {childTimeTup}, {timestampTup}, {childTimeList}")
                childTimeList[index] = (childTimeTup[0], timestampTup[1])
                return
            #       |-----|
            # |---|
            # This is the case where we have a timestamp tuple that occurs before the current tuple we're checking
            # while there is no overlap
            # This may be due to the order in which we checked the child functions
            # One child function may have timestamps like so: (1,2) (10,15) then the next child function will have a
            # timestamp like so: (3,5)
            # Due to the nature of this algorithm, it will appear as if a timestamp appears earlier
            # However, we know there is no overlap, but the above cases assume overlap, thus we need to handle this case
            else:
                childTimeList.insert(index, childTimeTup)
                return
        #   |-----|
        #         |---|
        # In this case, we just need to extend the first tuple to the right with the second tuple
        elif childTimeTup[0] == timestampTup[1]:
            # print(f"Woah, {childTimeTup}, {timestampTup}")
            childTimeList[index] = (timestampTup[0], childTimeTup[1])
            return
        # |-------| or  |------|   or |------|
        #   |---|          |---|          |-----|
        # The third case is what we handle by extending to the right,
        # the other cases means we need to go further down the list
        elif childTimeTup[0] < timestampTup[1]:
            if childTimeTup[1] > timestampTup[1]:
                # print(f"Woo, {childTimeTup}, {timestampTup}")
                childTimeList[index] = (timestampTup[0], childTimeTup[1])
                return
            # Will continue to the next iteration if does not pass the above test
        #   |-----| |----|
        # This case will only mean we need to append to the list only if we have reached the end of the list
        # meaning that there is no possible overlap for any of the other timestamps
        elif timestampTup == childTimeList[-1]:
            childTimeList.append(childTimeTup)
            return
        index += 1

'''
This function will update the execTime attribute of the tree nodes to be the node's actual execution time
A function's actual execution time will be its original execution time (that was calculated from the timestamps that were
recorded with the trace/sample) minus total execution time spent through the function's child functions.

This may not be the correct solution as this is based off my initial timestamp merging solution in
FunctionTree.updateTimestamps
My new solution will most likely change this significantly ***
V
V
This total execution time is determined by taking the earliest timestamp that occurred within the parent function's
execution time (>= the parent function's start timestamp) and the latest timestamp that occurred 
within the parent function's execution time (<= parent function's end timestamp). Then the total execution time
is the difference between those two timestamps we found.

***
This function will perform the redistribution of the function execution times in order to obtain the true
execution times of the function calls

The basic idea is to sum up all of the execution times of child functions without including overlap and sections
of time where execution does not occur for the function
Then subtract this sum from the parent function's execution time thus resulting in the time that was spent only
in the parent function.
'''
def updateExecTime(tree):
    childTimestampList = []
    old = tree.execTime
    # The case where we reach a leaf, then do nothing, the execution time of the leaf that's been calculated
    # is the true execution time of that function
    if not tree.childFuncs:
        return
    # For each child function of the current node, we will traverse down into its child nodes
    # This will retrieve the earliest and latest timestamp seen in the child functions
    for child in tree.childFuncs:
        updateExecTime(child)
        for childTimestamps in child.timestamps:
            for treeTimestamps in tree.timestamps:
                # Check if there has been timestamps from the child functions added yet
                if not childTimestampList:
                    if childTimestamps[0] < treeTimestamps[0]:
                        print("ERROR: Child function somehow executed earlier than parent function, exiting...")
                        sys.exit()
                    elif childTimestamps[1] > treeTimestamps[1] and treeTimestamps == tree.timestamps[-1]:
                        print("Uhh", treeTimestamps, childTimestamps)
                        print("ERROR: Child function somehow terminated after the parent function, exiting...")
                        sys.exit()
                    # If this point is reached, we know the current child timestamp tuple occurred
                    # During the execution of the parent function
                    # Thus we append the tuple to the timestamp list
                    # There is no need to traverse the tree timestamp list since
                    # That list is sorted in ascending order,
                    # therefore the timestamps we appended will always trigger the above conditions
                    # And we would waste time checking
                    else:
                        childTimestampList.append(childTimestamps)
                        break
                else:
                    if childTimestamps[0] < treeTimestamps[0]:
                        print("ERROR: Child function somehow executed earlier than parent function, exiting...")
                        sys.exit()
                    elif childTimestamps[1] > treeTimestamps[1] and treeTimestamps == tree.timestamps[-1]:
                        print("Uhh", treeTimestamps, childTimestamps)
                        print(f"Tree:{tree} \n Child: {child}")
                        print("ERROR: Child function somehow terminated after the parent function, exiting...")
                        sys.exit()
                    # If this point is reached, we know the current child timestamp tuple occurred
                    # During the execution of the parent function
                    # Thus we append the tuple to the timestamp list
                    # There is no need to traverse the tree timestamp list since
                    # That list is sorted in ascending order,
                    # therefore the timestamps we appended will always trigger the above conditions
                    # And we would waste time checking
                    else:
                        updateTimestampList(childTimestamps, childTimestampList)
                        break

    # Check if the child timestamps tuple was set, if so, then we update the execution time of the current node
    for timeTup in childTimestampList:

        tree.execTime -= timeTup[1] - timeTup[0]
    # Error check where somehow the execution time is changed to a negative value which makes no sense
    if tree.execTime < 0:
        print(f"poop {tree} \n {childTimestampList} \n {old}")
        sys.exit()
    return
# Redistributes Time info to have the execution time attribute of each node represent the true execution of the function
# NOTE: I wrote this in Converter because I am assuming that Scaler will output raw data in the form of fakeData.json
# Where execution time is already calculated based off the timestamps given
# This execution time is what gets redistributed
# IF this ends up not being the case and Steven automatically performs the redistribution, then this will be moved into DataAggregator_V2
def redistributeExecTime(tidData):
    for treeDict in tidData.values():
        for tree in treeDict.values():
            updateExecTime(tree)
    return

# This is simply a helper function for retrieving the function information of nodes from a single function tree
def getFuncsfromTree(tree):
    if not tree.childFuncs:
        return {tree.func: tree.execTime}
    funcDict = {}
    for child in tree.childFuncs:
        funcDict.update(getFuncsfromTree(child))
    funcDict.update({tree.func: tree.execTime})
    return funcDict

# This function will return a dictionary of all of the functions and their execution times
# If the execution times have been redistributed then the sum of all of the execution times in this dictionary
# will represent the total ON-CPU execution time
# We have no idea about Off-cpu execution time just from this information
def retrieveFuncs(tidData):
    funcTimeDict = {}
    for treeDict in tidData.values():
        for tree in treeDict.values():
            if not funcTimeDict:
                funcTimeDict = getFuncsfromTree(tree)
                # print(funcTimeDict)
                # printTree(treeDict)
            else:
                # This will merge the two dictionaries by summing any values of the same key and also adding keys that
                # do not yet exist along with their associated values from their respective dictionary
                # print(tempDict)
                for func in set(tempDict := getFuncsfromTree(tree)) | set(funcTimeDict):
                    funcTimeDict[func] = tempDict.get(func, 0.0) + funcTimeDict.get(func, 0.0)
    return funcTimeDict



def main():
    root = tk.Tk()
    root.withdraw()
    fileName = filedialog.askopenfilename()
    if fileName == '':
        # If no file name then just default to opening a file in the repo
        # print(True)
        fileName = "perfMemcachedData_V2.json"
    with open(fileName, 'r') as j_file:
        tidData = json.load(j_file)

    # By default, samples are used, we let the user use timestamps if they want to
    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        global useTimestamps
        useTimestamps = True

    # NOTE: Below the code currently is doing work on timestamps
    # If useTimestamps is not set to True, then this will crash

    loadData(tidData)
    outDict = {}

    # print the first tree
    for treeDict in tidData.values():
        printTree(treeDict)
        break

    # Retrieve the root execution times before redistribution
    # These execution times do not represent pure On-CPU execution.
    # This is explained in comments above this function's definition
    outDict["ThreadRoot"] = getThreadRootExe(tidData)
    # Positional Debug print to let me know where we are in the code
    print("Wow")

    redistributeExecTime(tidData)

    # pprint.pprint(tidData)
    # print(tidData)

    # Print the first tree again, but this time the tree has had its execution times redistributed
    for treeDict in tidData.values():
        printTree(treeDict)
        break
    # Retrieve the TOTAL time spent in each thread, this is a combination of On-CPU and Off-CPU execution times
    outDict["threads"] = getThreadTotalExecution(tidData)
    print(outDict["threads"])

    # Retrieve the function execution times
    # At this point, the execution times have been redistributed
    # Therefore, the sum of these execution times should be the On-CPU execution times
    outDict["funcs"] = retrieveFuncs(tidData)
    print(outDict["funcs"])
    print("sum:", sum(outDict["funcs"].values()))

    # As explained, these execution times simply represent the pre-redistribution
    # root execution times, I wanted to know if the sum of the function redistributed execution times
    # would be the same as the sum of these root times
    # I discovered they are not, but it makes sense, This is explained in the get thread root execution time function
    # comments
    print(outDict["ThreadRoot"])
    print("sum:", sum(outDict["ThreadRoot"].values()))


    # print(getThreadSampTot(tidData))
    '''
    leafDict = getLeaves(tidData)
    #pprint.pprint(leafDict)
    libDict = getLibSampTot(leafDict)
    #pprint.pprint(libDict)
    libTotDict = {}
    for libraryDict in libDict.values():
        for lib, total in libraryDict.items():
            libTotDict.setdefault(lib, 0)
            libTotDict[lib] += total
    '''
    # Dump our output data into the chart data json so we can chart stuff
    with open("chart_data.json", 'w') as chart_file:
        json.dump(outDict, chart_file, indent=4)


if __name__ == "__main__":
    main()