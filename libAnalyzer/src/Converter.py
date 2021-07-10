
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
                tidDict[tid] = tree.timestamps
            # If the current root's timestamp starts earlier than the current thread timestamp
            elif tree.timestamps[0] < tidDict[tid][0]:
                # If the start timestamp is earlier and the end timestamp is later, then set the thread's timestamp
                # as the current root's timestamp
                if tree.timestamps[1] > tidDict[tid][1]:
                    tidDict[tid] = tree.timestamps
                # If the current root's start timestamp is earlier, but the end timestamp is earlier or the same
                # as the thread's end timestamp
                # Then simply replace the thread's start timestamp
                else:
                    tidDict[tid] = (tree.timestamps[0], tidDict[tid][1])
            # If the current root's start timestamp is later or the same as the thread's start timestamp
            # But the root's end timestamp is later, then replace the end timestamp of the thread
            elif tree.timestamps[1] > tidDict[tid][1]:
                tidDict[tid] = (tidDict[tid][0], tree.timestamps[1])
    # After we get the timestamps, calculate the execution times
    for tid, time in tidDict.items():
        tidDict[tid] = time[1]-time[0]
    return tidDict

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
My new solution may most likely be:
N/A
'''
def updateExecTime(tree):
    childTimestamps = ()
    # The case where we reach a leaf, then do nothing, the execution time of the leaf that's been calculated
    # is the true execution time of that function
    if not tree.childFuncs:
        return
    # For each child function of the current node, we will traverse down into its child nodes
    # This will retrieve the earliest and latest timestamp seen in the child functions
    for child in tree.childFuncs:
        updateExecTime(child)
        # If we have not yet set our child timestamps tuple, set it with the current child's timestamps
        if not childTimestamps:
            # We then will not use any timestamps that had a start timestamp
            # that started before the current node's start timestamp
            # As that would mean we are counting time that the current node never executed in
            # This should not be possible as child functions should always run after the start of the parent function
            if child.timestamps[0] < tree.timestamps[0]:
                print("ERROR: Child Function somehow ran before the execution of the parent function, exiting...")
                sys.exit()
            else:
                childTimestamps = child.timestamps
        # If the child's start timestamp is earlier than the current earliest timestamp we have found
        elif child.timestamps[0] < childTimestamps[0]:
            if child.timestamps[0] < tree.timestamps[0]:
                print("ERROR: Child Function somehow ran before the execution of the parent function, exiting...")
                sys.exit()
            else:
                # If the child's end timestamp is later than the latest timestamp found, set the current timestamp tuple
                # as the current child's timestamps
                if child.timestamps[1] > childTimestamps[1]:
                    childTimestamps = child.timestamps
                # Otherwise, we know that the child's start timestamp is earlier than the current earliest timestamp
                # and the end timestamp is earlier or equal to the current latest timestamp
                # Thus set the earliest timestamp as the current child's start timestamp
                else:
                    childTimestamps = (child.timestamps[0], childTimestamps[1])
        # Otherwise, we know that the child's start timestamp occurs later than the current earliest timestamp
        # Then we need to check if the current child's end timestamp occurs later than the latest timestamp
        # If so then set the latest timestamp as the child's end timestamp

        # If that is not the case, then we know that the child's start timestamp occurs later than the earliest
        # timestamp and the end timestamp occurs earlier than the latest timestamp
        # Thus we discard those timestamps
        elif child.timestamps[1] > childTimestamps[1]:
            childTimestamps = (childTimestamps[0], child.timestamps[1])
    # Check if the child timestamps tuple was set, if so, then we update the execution time of the current node
    if childTimestamps:
        tree.execTime -= childTimestamps[1] - childTimestamps[0]
    # Error check where somehow the execution time is changed to a negative value which makes no sense
    if tree.execTime < 0:
        print("poop", tree, childTimestamps, old)
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
                print(funcTimeDict)
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

    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        global useTimestamps
        useTimestamps = True

    loadData(tidData)
    outDict = {}
    for treeDict in tidData.values():
        printTree(treeDict)
        break
    print("Wow")
    redistributeExecTime(tidData)
    # pprint.pprint(tidData)
    # print(tidData)
    # for treeDict in tidData.values():
    #    printTree(treeDict)
    #    break
    outDict["threads"] = getThreadTotalExecution(tidData)
    print(outDict["threads"])
    outDict["funcs"] = retrieveFuncs(tidData)
    print(outDict["funcs"])
    print("sum:", sum(outDict["funcs"].values()))


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
    with open("chart_data.json", 'w') as chart_file:
        json.dump(outDict, chart_file, indent=4)


if __name__ == "__main__":
    main()