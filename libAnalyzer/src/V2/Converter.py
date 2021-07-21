
from DataAggregator_V2 import FunctionTree, printTree
import pprint
import json
import tkinter as tk
from tkinter import filedialog
import sys
import copy
from functools import wraps
'''
This converter script will convert the final output data of Data Aggregator into an easy to use format
for charter, that way I pre-process all of the data so I can immediately create a pie chart.
Currently it will generate a dictionary of sample totals for libraries called for a specific thread

Data Retrieval: 
    - Total Thread Samples:
      Sum up sample totals of all root nodes
    - Function Sample Totals:
      Sum up sample totals of nodes with same function after sample redistribution
    - Library Sample Totals:
      Same principle, sum up sample totals of nodes that have the same library
    - If maintaining thread specificity:
      Same idea as above, except just keeping the sample totals of trees that are a part of different threads seperate
    
    - Timing info
      - Thread Execution Times
        Two Possibilities:
            - Find the earliest and latest timestamp seen in the thread's data and calculate execution time from that
            - Sum up all of the execution times of all of the roots of the trees stored for a thread
      - Functions and Libraries Execution Times
        Perform redistribution and then we can sum all of the functions together to get total execution time per function/library
      - If maintaining thread specificity:
        Same idea as the samples, since the input data structure is thread specific, it is easy to keep data separated by thread
Redistribution:
  - Samples:
    Simply subtract the sample totals of the direct children from the parent's sample total before their sample totals have been redistributed
  - Time:
    Simply subtract the execution time of children after avoiding overlaps and other issues from the parent's execution time before the children's execution times have been redistributed

At the moment, total execution time may be misrepresented given that I have not accounted for true concurrent threads.
The current underlying assumption is that the data is sequential, thus execution time overlap does not happen.
'''

useTimestamps = False

# TODO Deal with how to show the data that was under threshold
# TODO Deal with repeating decimals like 33.333333333% => 1/3 of 100
def useThreshold(_func=None, *, separateTID=False, threshold=5):
    """
    Goal of use threshold decorator:
    Will call a function that should return a dictionary with data in it (like the data retrieval functions in this script)

    This decorator will then add functionality to the function by doing post processing of stripping
    any data that does not exceed the threshold (This threshold will be based off of execution time).
    Then it will return a new dictionary with only the data that exceeded the threshold.
    So if a piece of data was only 4% of the total execution time while we are looking for data that is >=5% then
    we will throw it away
    -------------------------------------------------------
    I am using a decorator because I do not want to alter the existing functions as we may have the need to use the
    original non-decorated function.

    We can do this by either removing the decorator or using the wraps decorator from functools
    With the wraps decorator, we can call the original un-decorated function through the .__wrapped__ attribute

    useThreshold has the @wraps decorated installed.

    So if we use useThresholds to decorate a function, then we can call both the decorated function and the original
    For example:
    In this script, I currently have retrieveAttrs decorated with useThreshold
    To call the original retrieveAttrs just do this:
    retrieveAttrs.__wrapped__(arg1, arg2, arg3)
    To call the decorated version, just do the usual with any decorated function:
    retrieveAttrs(arg1, arg2, arg3)
    ------------------------------------------------------
    Input Assumptions:
    {tid:{lib:{func:execution time or sample totals}}}
    or
    {lib:{exec time or sample totals}}
    or
    {func:{exec time or sample totals}}
    or
    {lib:{func:execution time or sample totals}}

    in general some form of tid:lib:func
    """
    # print("in use threshold")
    def get_sum(dictData):
        """For calculating a sum of all values from a dictionary, can be nested or not"""
        sum = 0
        for val in dictData.values():
            if isinstance(val, dict):
                sum += get_sum(val)
            else:
                sum += val
        return sum


    def processDict(aDict, summ, thresholdVal):
        """This will process any dictionary, nested or not, to strip any values that <= the thresholdVal"""
        output = {}
        for key, val in aDict.items():
            if isinstance(val, dict):
                output.setdefault(key, {})
                retVal = processDict(val, summ, thresholdVal)
                if retVal:
                    output[key].update(retVal)
                else:
                    del output[key]
            else:
                calcVal = val/summ * 100  # Convert the data value to a percentage
                if calcVal >= thresholdVal:
                    output[key] = calcVal
        return output

    def processDictwithTIDSep(aDict, thresholdVal):
        """
        This will only accept dictionaries that have TIDs in them.
        If they exist then we will process the dictionaries at the thread level.
        Therefore the resulting percentages represent the thread execution time (or sample count),
        not total execution time (or total sample count)
        """
        output = {}
        for key, val in aDict.items():
            try:
                int(key.split("/")[-1])
            except ValueError:
                raise Exception("TID does not exist.")
            summ = get_sum(val) # Calculate the thread execution time or thread sample count
            if isinstance(val, dict):
                output[key] = processDict(val, summ, thresholdVal)
            else:
                raise Exception("TID has no functions or libraries.")
            newSum = get_sum(output[key])
            output[key][f"Below threshold: {str(thresholdVal)} percent"] = 100 - newSum
        return output

    def decorator_useThreshold(func):
        """Decorates the function func"""
        # print("In decorator use threshold")
        @wraps(func)  # For retaining the original function
        def inner(*args, **kwargs):
            """The decorated function"""
            # print("Inside inner")
            retVal = func(*args, **kwargs)

            if separateTID:  # If the user wants to process at the thread level, then do so
                retVal = processDictwithTIDSep(retVal, threshold)

            else:  # otherwise, just process based on total execution time
                summ = get_sum(retVal)  # calculate the total execution time from the data
                # print("WOOO", summ)
                retVal = processDict(retVal, summ, threshold)
                newSum = get_sum(retVal)
                retVal[f"Under threshold: {str(threshold)} percent"] = 100 - newSum
            return retVal  # return the processed dictionary
        return inner  # return the inner function which represents the decorated function
    # If none of the default parameters were passed in, then return the decorated function with the original function
    # passed in
    # otherwise, the default parameters were set, then return the decorated function
    if _func is None:
        return decorator_useThreshold
    else:
        return decorator_useThreshold(_func)

# This takes in the dictionary stored in the json format and reconstructs the original output data structure
# The original output data structure being: {tid: {root function name: function tree}}
def loadData(tidDict):
    for key, treeDict in tidDict.items():
        for treeKey, tree in treeDict.items():
            tidDict[key][treeKey] = FunctionTree.reconstructNodefromDict(tree, useTimestamps)

# ================================ DATA REDISTRIBUTION/PROCESSING FUNCTIONS ==========================================

# This function updates that timestamp list in the update exec time function
# by adding the timestamp if no overlap occurs to the list
# or by extending timestamps when overlaps occur
def updateTimestampList(childTimeList):
    # TODO: Shouldn't be required
    sorted(childTimeList)
    # (currentStart, currentEnd) will be the new merged timestamp tuple
    currentStart = 0
    currentEnd = 0

    # startInd will be the index of the first tuple we're merging from
    # tupleCount will be the offset from the start index as well as the count of the number of tuples being merged
    tupleCount = 0

    # As we merge tuples we will append them to the list, any tuples that were not merged will simply be appended
    # to the list as well.
    newList = []
    for timestampTup in childTimeList:
        # If currentEnd has been reset, then assign the current tuple.
        if currentEnd == 0:
            currentStart, currentEnd = timestampTup[0], timestampTup[1]
            tupleCount += 1
            continue

        # If the next tuple's start timestamp = the previous tuple's end timestamp, then we will extend our current
        # merged tuple with the end timestamp of the current tuple
        if currentEnd == timestampTup[0]:
            currentEnd = timestampTup[1]
            # TODO: Review: This should not happen since aggregator already merged such case
            if timestampTup == childTimeList[-1]:
                newList.append((currentStart, currentEnd))
        # Otherwise, we need to save the current merged tuple along with the indices of the tuples we will replace
        else:
            # Once we detect that the previous end timestamp does not match the current start timestamp
            # We will add the merged timestamps to the new list then reassign the current start timestamp
            # and end timestamp to the merging timestamp variables
            # TODO: Review: Shouldn't this be in the data aggregator script?
            newList.append((currentStart, currentEnd))
            currentStart, currentEnd = timestampTup[0], timestampTup[1]
            tupleCount = 0
            if timestampTup == childTimeList[-1]:
                newList.append((currentStart, currentEnd))

        tupleCount += 1

    # Edge case where there was one element in the time list, we simply append the currentStart and current end tuple
    # to the new list.
    if not newList and tupleCount == len(childTimeList):
        newList.append((currentStart, currentEnd))

    return newList

'''
This function will update the execTime attribute of the tree nodes to be the node's actual execution time
A function's actual execution time will be its original execution time (that was calculated from the timestamps that were
recorded with the trace/sample) minus total execution time spent through the function's child functions.

This function will perform the redistribution of the function execution times in order to obtain the true
execution times of the function calls

The basic idea is to sum up all of the execution times of child functions without including overlap and sections
of time where execution does not occur for the function
Then subtract this sum from the parent function's execution time thus resulting in the time that was spent only
in the parent function.
'''
def updateExecTime(tree):
    # print(tree)
    childTimestampList = []
    # old = tree.execTime
    # The case where we reach a leaf, then do nothing, the execution time of the leaf that's been calculated
    # is the true execution time of that function
    if not tree.childFuncs:
        return
    # For each child function of the current node, we will traverse down into its child nodes
    # This will retrieve the earliest and latest timestamp seen in the child functions
    for child in tree.childFuncs:
        updateExecTime(child)
        for childTS in child.timestamps:
            childTimestampList.append(childTS)

    # TODO: Review: I don't think it's necessary
    childTimestampList = updateTimestampList(childTimestampList)
    # Check if the child timestamps tuple was set, if so, then we update the execution time of the current node
    # a = 0
    for timeTup in childTimestampList:
        tree.execTime -= timeTup[1] - timeTup[0]
        # a += timeTup[1] - timeTup[0]
    # Error check where somehow the execution time is changed to a negative value which makes no sense
    if tree.execTime < 0:
        # TODO: Review: Why should this be possible
        # print(f"original exec time: {old}\nchild total exec time: {a}\ntree: {tree}\nchild timestamps: {childTimestampList} ")
        print("ERROR: Execution time is negative, exiting...")
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

def updateSamples(tree):
    childTotal = 0
    if not tree.childFuncs:
        return

    for child in tree.childFuncs:
        childTotal += child.sampleTotal
        updateSamples(child)

    tree.sampleTotal -= childTotal
    return

def redistributeSamples(tidData):
    for treeDict in tidData.values():
        for tree in treeDict.values():
            updateSamples(tree)
    return

# ========================================= DATA RETRIEVAL FUNCTIONS ===========================================

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
            if tidDict[tid] is not None:

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
            if tidDict[tid] is not None:
                tidDict[tid] += tree.execTime
            else:
                tidDict[tid] = tree.execTime
    return tidDict

# Will retrieve each thread's total execution time
# TODO: Review: Why not just read root node?
# The total execution time of the thread is determined by retrieving the earliest timestamp from all of the roots
# And retrieving the latest timestamp from all of the roots
# to calculate thread execution time
def getThreadTotalExecution(tidData):
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            # print(tree.timestamps)
            # If there is nothing in the dictionary, then set the current tree's timestamps as the current thread's timestamp
            if tidDict[tid] is None:
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

# ============================== SUBSECTION: Execution time Data Retrieval =======================

# Retrieves all of the samples associated with a function. Stores these with respect to the function's library
# and the thread it was called from
@useThreshold(separateTID=True)
def retrieveLibsFuncs(tidData):
    # Helper function to traverse the tree and retrieve execution times and
    # store them with respect to a function and library
    def getLibsFuncsfromTree(node):
        if not node.childFuncs:
            return {node.lib: {node.func: node.execTime}}
        funcDict = {}
        for child in node.childFuncs:
            # This nested for loop will merge function sample totals by summing them together
            for lib in set(innertempDict := getLibsFuncsfromTree(child)) | set(funcDict):
                # Tests for if the library exists as a key yet in the dictionaries
                innertempDict.setdefault(lib, {})
                funcDict.setdefault(lib, {})
                for func in set(innertempDict[lib]) | set(funcDict[lib]):
                    funcDict[lib][func] = innertempDict[lib].get(func, 0.0) + funcDict[lib].get(func, 0.0)
        # Another test for if the library exists yet in the dictionary
        try:
            funcDict[node.lib][node.func] = funcDict[node.lib].get(node.func, 0.0) + node.execTime
        except KeyError:
            funcDict[node.lib] = {}
            funcDict[node.lib][node.func] = funcDict[node.lib].get(node.func, 0.0) + node.execTime
        return funcDict

    libfuncDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if libfuncDict[tid] is None:
                libfuncDict[tid] = getLibsFuncsfromTree(tree)
            else:
                # This nested for loop is for merging the dictionaries by summing together sample totals for a function
                for lib in set(tempDict := getLibsFuncsfromTree(tree)) | set(libfuncDict[tid]):
                    # Tests for if the library is in the dictionary
                    tempDict.setdefault(lib, {})
                    libfuncDict[tid].setdefault(lib, {})
                    # print(f"temp dict: {tempDict}")
                    # print(f"func sample Dict: {funcSampDict}")
                    for func in set(tempDict[lib]) | set(libfuncDict[tid][lib]):
                        # print(func)
                        libfuncDict[tid][lib][func] = tempDict[lib].get(func, 0.0) + libfuncDict[tid][lib].get(func, 0.0)
    return libfuncDict

# ==========================================================================================

'''
This function will construct a dictionary for attrKey with the sum of all attrVals associated with attrKey.
For example, in FunctionTree, attrKey can be "func" and attrVal can be "execTime".
This means that this function will retrieve the execution time and function name of each node in the function tree
and set the function name as the key of a dictionary and the execution time as the value.

We perform merging of dictionaries with this rule:
Given 2 dictionaries, if there are identical keys between the two, sum their values together, otherwise just copy the
key-value pair over to the newly merged dictionary

After redistribution, the values in the resulting dictionaries represent the true total On-CPU Execution time
for their respective keys (If "execTime" was passed for attrVal)

If "sampleTotal" is passed to attrVal, then it simply means that any attrKeys that have a non-zero positive 
value, were the end of a unique call stack that was sampled.
'''

@useThreshold
def retrieveAttrs(tidData, attrKey, attrVal):
    # This is a helper function to retrieve a dictionary of the same form as described for
    # retrieveAttrs from a tree structure
    def getAttrsfromTree(node, attrKey, attrVal):
        if not node.childFuncs:
            return {getattr(node, attrKey): getattr(node, attrVal)}
        attrDict = {}

        # Iterate through each child and merge the current node's dictionary and the child's dictionary
        # The current node's dictionary is attrDict,
        # and the child's dictionary is from the recursive call of getAttrsfromTree()
        for child in node.childFuncs:
            for key in set(innertempDict := getAttrsfromTree(child, attrKey, attrVal)) | set(attrDict):
                attrDict[key] = innertempDict.get(key, 0.0) + attrDict.get(key, 0.0)

        # We have yet to add in the current node's value, thus we add it in.
        attrDict[getattr(node, attrKey)] = attrDict.get(getattr(node, attrKey), 0.0) + getattr(node, attrVal)

        return attrDict
    retDict = {}
    for treeDict in tidData.values():
        for tree in treeDict.values():
            if not retDict:
                retDict = getAttrsfromTree(tree, attrKey, attrVal)
            else:
                for key in set(tempDict := getAttrsfromTree(tree, attrKey, attrVal)) | set(retDict):
                    retDict[key] = tempDict.get(key, 0.0) + retDict.get(key, 0.0)
    return retDict

# If we are not seperating by TIDs, then simply pass in the function name to be decorated
# If separating by TIDs, then pass in separateTID = True as the parameter for useThreshold and then call the return
# value of useThreshold with the function name passed in
# Like so: useThreshold_RetrieveAttrs = useThreshold(separateTID=True)(retrieveAttrs)
# useThreshold_RetrieveAttrs = useThreshold(retrieveAttrs)

# ===================== SUB SECTION: Sample data retrieval =================================


@useThreshold(separateTID=True)
def retrieveFuncSamples(tidData):
    """
    Retrieves all of the samples associated with a function. Stores these with respect to the function's library
    and the thread it was called from
    """
    # print("AAAA")
    def getFuncsSampsfromTree(tree):
        """Helper function for retrieving function samples while also maintaining library specificity"""
        if not tree.childFuncs:
            return {tree.lib: {tree.func: tree.sampleTotal}}
        funcDict = {}
        for child in tree.childFuncs:
            # This nested for loop will merge function sample totals by summing them together
            for lib in set(innertempDict := getFuncsSampsfromTree(child)) | set(funcDict):
                # Tests for if the library exists as a key yet in the dictionaries
                innertempDict.setdefault(lib, {})
                funcDict.setdefault(lib, {})
                for func in set(innertempDict[lib]) | set(funcDict[lib]):
                    funcDict[lib][func] = innertempDict[lib].get(func, 0.0) + funcDict[lib].get(func, 0.0)
        # Another test for if the library exists yet in the dictionary
        try:
            funcDict[tree.lib][tree.func] = funcDict[tree.lib].get(tree.func, 0.0) + tree.sampleTotal
        except KeyError:
            funcDict[tree.lib] = {}
            funcDict[tree.lib][tree.func] = funcDict[tree.lib].get(tree.func, 0.0) + tree.sampleTotal
        return funcDict

    funcSampDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if funcSampDict[tid] is None:
                funcSampDict[tid] = getFuncsSampsfromTree(tree)
            else:
                # This nested for loop is for merging the dictionaries by summing together sample totals for a function
                for lib in set(tempDict := getFuncsSampsfromTree(tree)) | set(funcSampDict[tid]):
                    # Tests for if the library is in the dictionary
                    tempDict.setdefault(lib, {})
                    funcSampDict[tid].setdefault(lib, {})
                    # print(f"temp dict: {tempDict}")
                    # print(f"func sample Dict: {funcSampDict}")
                    for func in set(tempDict[lib]) | set(funcSampDict[tid][lib]):
                        funcSampDict[tid][lib][func] = tempDict[lib].get(func, 0.0) + funcSampDict[tid][lib].get(func, 0.0)
    return funcSampDict


@useThreshold(separateTID=True)
def retrieveTIDLibSamples(tidData):
    """This function will return a dictionary of all of the libraries and their samples"""
    # This is simply a helper function for retrieving the library information of nodes from a single function tree
    def getLibsSampsfromTree(tree):
        if not tree.childFuncs:
            # print(tree.lib, tree.sampleTotal)
            return {tree.lib: tree.sampleTotal}
        libDict = {}
        for child in tree.childFuncs:
            for lib in set(innertempDict := getLibsSampsfromTree(child)) | set(libDict):
                libDict[lib] = innertempDict.get(lib, 0.0) + libDict.get(lib, 0.0)
        libDict[tree.lib] = libDict.get(tree.lib, 0.0) + tree.sampleTotal
        return libDict

    libSampDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if libSampDict[tid] is None:
                libSampDict[tid] = getLibsSampsfromTree(tree)
            else:
                # This will merge the two dictionaries by summing any values of the same key and also adding keys that
                # do not yet exist along with their associated values from their respective dictionary
                for lib in set(tempDict := getLibsSampsfromTree(tree)) | set(libSampDict[tid]):
                    libSampDict[tid][lib] = tempDict.get(lib, 0.0) + libSampDict[tid].get(lib, 0.0)
    return libSampDict

# ==============================================================================================

# ================= Helper function to convert the {TID:{Lib:{Func:sample/execution time}}} =============

# This function will convert {TID:{Lib:{Func:sample/execution time}}} into {lib:{func:execution time/samples}}
def groupLibsandFuncs(TIDLibFuncDict):
    # pprint.pprint(TIDLibFuncDict)
    newLibFuncDict = {}
    for libFuncDict in TIDLibFuncDict.values():
        if not newLibFuncDict:
            newLibFuncDict = copy.deepcopy(libFuncDict)
        else:
            for lib, funcDict in libFuncDict.items():
                newLibFuncDict.setdefault(lib, {})
                libFuncDict.setdefault(lib, {})
                for func in set(libFuncDict) | set(newLibFuncDict):
                    newLibFuncDict[lib][func] = libFuncDict[lib].get(func, 0.0) + newLibFuncDict[lib].get(func, 0.0)
    # pprint.pprint(TIDLibFuncDict)
    return newLibFuncDict

# ===================================================================================

# =============================== Debugging Functions ===============================
def printFirstTreeDict(tidData):
    for treeDict in tidData.values():
        printTree(treeDict)
        # break
    return

# ===================================================================================
def main():
    print("in main")
    root = tk.Tk()
    root.withdraw()
    fileName = filedialog.askopenfilename()
    # TODO: Review: Maybe version problem
    if fileName == ():
        # If no file name then just default to opening a file in the repo
        # print(True)
        fileName = "/home/st/Projects/Scaler/libAnalyzer/src/V2/perfMemcachedData_V2.json"
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
    # printFirstTreeDict(tidData)
    outDict = {}
    if useTimestamps:
        # print the first tree
        #printFirstTreeDict(tidData)

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
        #printFirstTreeDict(tidData)
        # Retrieve the TOTAL time spent in each thread, this is a combination of On-CPU and Off-CPU execution times
        outDict["threads"] = getThreadTotalExecution(tidData)
        print(outDict["threads"])

        # Retrieve the function execution times
        # At this point, the execution times have been redistributed
        # Therefore, the sum of these execution times should be the On-CPU execution times
        # outDict["funcs"] = retrieveFuncs(tidData)
        print("what")
        outDict["funcs"] = retrieveAttrs.__wrapped__(tidData, "func", "execTime")
        print(outDict["funcs"])
        print("sum:", sum(outDict["funcs"].values()))

        print("woop")
        # print(useThreshold_RetrieveAttrs(tidData, "func", "execTime"))
        print(retrieveAttrs(tidData, "func", "execTime"))
        # print("Wow")

        # print(outDict["funcs"] == retrieveAttr(tidData, "func", "execTime"))

        # As explained, these execution times simply represent the pre-redistribution
        # root execution times, I wanted to know if the sum of the function redistributed execution times
        # would be the same as the sum of these root times
        # I discovered they are not, but it makes sense, This is explained in the get thread root execution time function
        # comments
        print(outDict["ThreadRoot"])
        print("sum:", sum(outDict["ThreadRoot"].values()))


        # outDict["libs"] = retrieveLibs(tidData)
        outDict["libs"] = retrieveAttrs.__wrapped__(tidData, "lib", "execTime")
        print(outDict["libs"])
        print("sum: ", sum(outDict["libs"].values()))
        print("uh")
        print(retrieveAttrs(tidData,"lib", "execTime"))
        # print("Woo")

        # print(outDict["libs"] == retrieveAttr(tidData, "lib", "execTime"))

        outDict["tidlibsFuncs"] = retrieveLibsFuncs.__wrapped__(tidData)
        pprint.pprint(retrieveLibsFuncs(tidData))
        outDict["libsFuncs"] = groupLibsandFuncs(outDict["tidlibsFuncs"])

    else:
        # Sum all thread samples and store them with their respective threads
        outDict["ThreadSamps"] = getThreadSampTot(tidData)
        print(outDict["ThreadSamps"])

        # Perform sample redistribution
        redistributeSamples(tidData)
        # printFirstTreeDict(tidData)
        # outDict["FuncSamps"] =

        # Retrieving all library samples and store them with their respective libraries and in their respective threads
        outDict["TIDLibSamps"] = retrieveTIDLibSamples.__wrapped__(tidData)
        # print("wow")
        print(outDict["TIDLibSamps"])

        # print(retrieveAttrs(tidData, "lib", "sampleTotal"))
        # print(outDict["TIDLibSamps"] == retrieveAttrs(tidData, "lib", "sampleTotal"))
        # Just to sum all of the library samples to see if they sum as expected
        # Should sum to the same amounts that is stored in outDict["ThreadSamps"]
        libSums = []
        for index, tid in enumerate(outDict["TIDLibSamps"].keys()):
            libSums.append(sum(outDict["TIDLibSamps"][tid].values()))
            print(f"Tid: {tid}, sum: {libSums[index]}")

        # print(sum(libSums) == sum(retrieveAttrs(tidData, "lib", "sampleTotal").values()))
        # This will sum all of the values in each thread of outDict["LibSamps"] and report the sample totals for each
        # library irrespective of threads
        for libDict in outDict["TIDLibSamps"].values():
            try:
                for lib in set(outDict["LibTotSamps"]) | set(libDict):
                    outDict["LibTotSamps"][lib] = outDict["LibTotSamps"].get(lib, 0.0) + libDict.get(lib, 0.0)
            except KeyError:
                outDict["LibTotSamps"] = libDict.copy()

        print(outDict["LibTotSamps"])
        print(outDict["TIDLibSamps"])

        # This will grab the sample totals of all functions and store them with their respective threads and libraries
        outDict["FuncLibSamps"] = retrieveFuncSamples.__wrapped__(tidData)

        pprint.pprint(retrieveTIDLibSamples(tidData))
        pprint.pprint(retrieveFuncSamples(tidData))
        # printFirstTreeDict(tidData)

    # Dump our output data into the chart data json so we can chart stuff
    with open("chart_data.json", 'w') as chart_file:
        json.dump(outDict, chart_file, indent=4)


if __name__ == "__main__":
    main()