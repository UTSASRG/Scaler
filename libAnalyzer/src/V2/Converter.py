
from DataAggregator_V2 import FunctionTree, printTree
import pprint
import json
import tkinter as tk
from tkinter import filedialog
import sys
import copy
from functools import wraps
import warnings

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

Things to consider for now:
Concurrency
Synchronization Timing (Lock acquisition, etc.)
Coz Implementation

Since concurrency is not being considered. Assuming that all timing data is sequential (therefore no overlap), then
after redistribution, the execution times of every node in the FunctionTree represents the true total execution time 
of the node / function. 

THEN, the sum of all the execution times in the tree represents the total ON-CPU execution time of the data.
'''

useTimestamps = False

def useThreshold(_func=None, *, separateTID=False, threshold=5):
    """
        Goal of use threshold decorator:
    Will call a function that should return a dictionary with data in it
    (like the data retrieval functions in this script)

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
    :param _func: An arbitrary function, it should return some kind of dictionary generated from tidData
    :param separateTID: A boolean, when set to True tells the function to process with respect to TID
                        Default value is: False
    :param threshold: A threshold value, has to be between 0 and 100
    :return: The decorated function
    """
    belowThresStr = f"Under threshold of {str(threshold)} percent"
    # print("in use threshold")
    def get_sum(dictData):
        """
        For calculating a sum of all values from a dictionary, can be nested or not
        :param dictData: An arbitary dictionary (can be nested or not), assuming that the values are real numbers
        :return: A sum of all values in the dictionary
        """
        sum = 0
        try:
            for val in dictData.values():
                if isinstance(val, dict):
                    sum += get_sum(val)
                else:
                    sum += val
        # Can happen with kernel attribution.
        except AttributeError:
            print(dictData)
        return sum

    def processDict(aDict, summ, thresholdVal):
        """
        This will process any dictionary, nested or not, to strip any values that <= the thresholdVal
        :param aDict: A dictionary
        :param summ: An integer or float that represents total execution time or total sample counts
                     (can be with respect to thread or all data in general)
        :param thresholdVal: A threshold value, generally is obtained from the outer function
        :return: The same as aDict but with all the values replaced with a percentage calculated based on the
                 original value and the summ value
        """
        output = {}
        for key, val in aDict.items():
            if isinstance(val, dict):
                output.setdefault(key, {})
                retVal = processDict(val, summ, thresholdVal)
                # Update the output dictionary with our returned dictionary
                # If the returned dictionary is empty, then simply delete the entry for key as it will just be an empty
                # dictionary
                if retVal:
                    # Check if the below threshold key was created
                    if belowThresStr in retVal:
                        # If the below threshold key is 0 then delete its entry
                        if not retVal[belowThresStr]:
                            del retVal[belowThresStr]
                    # Since we just changed the retVal dictionary, check again for if it is not empty
                    # if so then update our dictionary, otherwise, delete the entry as it will
                    # only have an empty dictionary associated with the key
                    if retVal:
                        output[key].update(retVal)
                    else:
                        del output[key]
                else:
                    del output[key]
            else:
                calcVal = val/summ * 100  # Convert the data value to a percentage
                if calcVal >= thresholdVal:
                    output.setdefault(key, 0)
                    output[key] += calcVal
                else:
                    output.setdefault(belowThresStr, 0)
                    output[belowThresStr] += calcVal
        return output

    def processDictwithTIDSep(aDict, thresholdVal):
        """
        This will only accept dictionaries that have TIDs in them.
        If they exist then we will process the dictionaries at the thread level.
        Therefore the resulting percentages represent the thread execution time (or sample count),
        not total execution time (or total sample count)
        :param aDict: A dictionary
        :param thresholdVal: A threshold value, generally is obtained from the outer function
        :return: A dictionary with the values replaced with a percentage representing a portion of the total
        samples or execution of their corresponding threads
        """
        output = {}
        for key, val in aDict.items():
            try:
                if key.startswith("0x"):
                    int(key, 16)
                else:
                    int(key.split("/")[-1])
            except ValueError:
                raise Exception("TID does not exist.")
            summ = get_sum(val) # Calculate the thread execution time or thread sample count
            if isinstance(val, dict):
                output[key] = processDict(val, summ, thresholdVal)
            else:
                # Just let this run, this may be triggered due to kernel attribution
                # But it is okay.
                warnings.warn("TID has no functions or libraries.")
                # raise Exception("TID has no functions or libraries.")
            # newSum = get_sum(output[key])
            # output[key][belowThresStr] = round(100 - newSum, 10)
        return output

    def decorator_useThreshold(func):
        """
        Decorates the function func
        :param func: A function which should return some form of dictionary that was generated from tidData
        :return: inner() which is the decorated form of the function func
        """
        # print("In decorator use threshold")
        @wraps(func)  # For retaining the original function
        def inner(*args, **kwargs):
            """
            The decorated function
            :param args: Arbitrary # of positional arguments
            :param kwargs: Arbitary # of keyword arguments
            :return: A dictionary in the original form of the output of func with the values replaced with
                     a percentage (represents a portion of total execution time either thread or all data in general)
                     along with all values that fell below the threshold grouped up into one element
            """
            # print("Inside inner")
            retVal = func(*args, **kwargs)

            if separateTID:  # If the user wants to process at the thread level, then do so
                retVal = processDictwithTIDSep(retVal, threshold)

            else:  # otherwise, just process based on total execution time
                summ = get_sum(retVal)  # calculate the total execution time from the data
                # print("WOOO", summ)
                retVal = processDict(retVal, summ, threshold)
                # newSum = get_sum(retVal)
                # retVal[belowThresStr] = round(100 - newSum, 10)
            return retVal  # return the processed dictionary
        return inner  # return the inner function which represents the decorated function
    # If none of the default parameters were passed in, then return the decorated function with the original function
    # passed in
    # otherwise, the default parameters were set, then return the decorated function
    if _func is None:
        return decorator_useThreshold
    else:
        return decorator_useThreshold(_func)


def loadData(tidDict):
    """
    This takes in the dictionary stored in the json format and reconstructs the original output data structure
    The original output data structure being: {tid: {root function name: function tree}}

    :param tidDict: A dictionary in the form of
                    {TID: {root function and library name: {func: function name,
                                                            lib: library name,
                                                            sampleTotal: sample count,
                                                            timestamps: (time start, time end)
                                                            execTime: execution time
                                                            childFuncs: []}
    :return: Nothing, updates the tidDict to the form of {TID:{root function and library name: Function Tree}}
    """
    for key, treeDict in tidDict.items():
        for treeKey, tree in treeDict.items():
            tidDict[key][treeKey] = FunctionTree.reconstructNodefromDict(tree, useTimestamps)

# ================================ DATA REDISTRIBUTION/PROCESSING FUNCTIONS ==========================================

# This function updates that timestamp list in the update exec time function
# by adding the timestamp if no overlap occurs to the list
# or by extending timestamps when overlaps occur
def updateTimestampList(childTimeList):
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
            if timestampTup == childTimeList[-1]:
                newList.append((currentStart, currentEnd))
        # Otherwise, we need to save the current merged tuple along with the indices of the tuples we will replace
        else:
            # Once we detect that the previous end timestamp does not match the current start timestamp
            # We will add the merged timestamps to the new list then reassign the current start timestamp
            # and end timestamp to the merging timestamp variables
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


def redistributeExecTime(tidData):
    """
    Redistributes Time info to have the execution time attribute of each node represent the true execution of the function
    NOTE: I wrote this in Converter because I am assuming that Scaler will output raw data in the form of fakeData.json
    Where execution time is already calculated based off the timestamps given
    This execution time is what gets redistributed
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return:
    """

    def updateExecTime(tree):
        """
        This function will update the execTime attribute of the tree nodes to be the node's actual execution time
        A function's actual execution time will be its original execution time (that was calculated from the timestamps that were
        recorded with the trace/sample) minus total execution time spent through the function's child functions.

        This function will perform the redistribution of the function execution times in order to obtain the true
        execution times of the function calls

        The basic idea is to sum up all of the execution times of child functions without including overlap and sections
        of time where execution does not occur for the function
        Then subtract this sum from the parent function's execution time thus resulting in the time that was spent only
        in the parent function.
        :param tree: A function tree node, generally a root node
        :return: Nothing, updates the function tree
        """
        # print(tree)
        childTimestampList = []
        # old = tree.execTime
        # The case where we reach a leaf, then do nothing, the execution time of the leaf that's been calculated
        # is the true execution time of that function
        if not tree.childFuncs:
            return
        # For each child function of the current node, we will traverse down into its child nodes
        # This will retrieve the earliest and latest timestamp seen in the child functions
        for child in tree.childFuncs.values():
            updateExecTime(child)
            for childTS in child.timestamps:
                childTimestampList.append(childTS)

        childTimestampList = updateTimestampList(childTimestampList)
        # Check if the child timestamps tuple was set, if so, then we update the execution time of the current node
        # a = 0
        for timeTup in childTimestampList:
            tree.execTime -= timeTup[1] - timeTup[0]
            # a += timeTup[1] - timeTup[0]
        # Error check where somehow the execution time is changed to a negative value which makes no sense
        if tree.execTime < 0:
            # print(f"original exec time: {old}\nchild total exec time: {a}\ntree: {tree}\nchild timestamps: {childTimestampList} ")
            print("ERROR: Execution time is negative, exiting...")
            sys.exit()
        return

    for treeDict in tidData.values():
        for tree in treeDict.values():
            updateExecTime(tree)
    return


def redistributeSamples(tidData):
    """
    Redistribute samples by subtracting the original sample count of all child functions from the parent's
    sample total
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: Nothing, updates tidData with the redistributed values
    """
    def updateSamples(tree):
        """
        Performs the redistribution for a tree
        :param tree: A function tree node, generally a root node
        :return: Nothing, updates the function tree
        """
        childTotal = 0
        if not tree.childFuncs:
            return

        for child in tree.childFuncs.values():
            childTotal += child.sampleTotal
            updateSamples(child)

        tree.sampleTotal -= childTotal
        return

    for treeDict in tidData.values():
        for tree in treeDict.values():
            updateSamples(tree)
    return

# ========================================= DATA RETRIEVAL FUNCTIONS ===========================================

def getThreadSampTot(tidData):
    """
    Retrieves the total samples recorded for each thread
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: a dictionary with a key-value pair of tid-total samples
    """
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if tidDict[tid] is not None:
                tidDict[tid] += tree.sampleTotal
            else:
                tidDict[tid] = tree.sampleTotal
    return tidDict

def getThreadRootExe(tidData):
    """
    Retrieves the execution times of the root functions
    If retrieved before redistribution, these execution times do not represent total On-CPU execution time
    This is because these root functions may include some off-cpu time in its total execution time
    This makes sense since there may be some I/O operations and the function has to wait or something like lock contention
    If retrieved after redistribution, total execution time retrieved represents total On-CPU Execution Time
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: A dictionary in the form of {TID:Execution Time}
    """
    tidDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if tidDict[tid] is not None:
                tidDict[tid] += tree.execTime
            else:
                tidDict[tid] = tree.execTime
    return tidDict


# ============================== SUBSECTION: Execution time Data Retrieval =======================

@useThreshold(separateTID=True)
def retrieveTIDLibsFuncs(tidData):
    """
    Retrieves all of the samples associated with a function. Stores these with respect to the function's library
    and the thread it was called from
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: a dictionary in the form of {TID:{Lib:Execution Time}}
    """

    def getTIDLibFuncsfromTree(node):
        """
        Helper function to traverse the tree and retrieve execution times and
        store them with respect to a function and library
        :param node: A Function Tree Node
        :return: A dictionary in the form of {TID:{Lib:Exec Time}}
        """
        if not node.childFuncs:
            return {node.lib: {node.func: node.execTime}}
        funcDict = {}
        for child in node.childFuncs.values():
            # This nested for loop will merge function sample totals by summing them together
            for lib in set(innertempDict := getTIDLibFuncsfromTree(child)) | set(funcDict):
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

    TIDLibFuncDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if TIDLibFuncDict[tid] is None:
                TIDLibFuncDict[tid] = getTIDLibFuncsfromTree(tree)
            else:
                # This nested for loop is for merging the dictionaries by summing together execution time for a function
                for lib in set(tempDict := getTIDLibFuncsfromTree(tree)) | set(TIDLibFuncDict[tid]):
                    # Tests for if the library is in the dictionary
                    tempDict.setdefault(lib, {})
                    TIDLibFuncDict[tid].setdefault(lib, {})
                    # print(f"temp dict: {tempDict}")
                    # print(f"func sample Dict: {funcSampDict}")
                    for func in set(tempDict[lib]) | set(TIDLibFuncDict[tid][lib]):
                        # print(func)
                        TIDLibFuncDict[tid][lib][func] = tempDict[lib].get(func, 0.0) + TIDLibFuncDict[tid][lib].get(func, 0.0)
    return TIDLibFuncDict

def retrieveTIDLibs(tidData):
    
    def getTIDLibsfromTree(node):
        # print(tidData)
        if not node.childFuncs:
            return {node.lib: node.execTime}
        libDict = {}
        for child in node.childFuncs.values():
            for lib in set(innerTempDict := getTIDLibsfromTree(child)) | set(libDict):
                libDict[lib] = innerTempDict.get(lib, 0.0) + libDict.get(lib, 0.0)
        libDict[node.lib] = node.execTime + libDict.get(node.lib, 0.0)
        return libDict

    TIDLibDict = dict.fromkeys(tidData.keys())
    for tid, treeDict in tidData.items():
        for tree in treeDict.values():
            if TIDLibDict[tid] is None:
                TIDLibDict[tid] = getTIDLibsfromTree(tree)
            else:
                for lib in set(tempDict := getTIDLibsfromTree(tree)) | set(TIDLibDict[tid]):
                    TIDLibDict[tid][lib] = tempDict.get(lib, 0.0) + TIDLibDict[tid].get(lib, 0.0)
    return TIDLibDict
    
    
# ==========================================================================================



@useThreshold
def retrieveAttrs(tidData, attrKey, attrVal):
    """
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

    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :param attrKey: Attribute name which be used to retrieve an attribute from a FunctionTree Node
                    That attribute will be the key to our returned dictionary (something like func or lib)
    :param attrVal: Attribute name which will be used to retrieve an attribute from a FunctionTree Node
                    That attribute will be used as the value for our returned dictionary (Like sampleTotal or execTime)
    :return: A dictionary with the key:value pair as {attrKey:attrVal}.
             With all attrKeys in tidData merged into one dictionary by summing their values
    """
    def getAttrsfromTree(node, attrKey, attrVal):
        """
        This is a helper function to retrieve a dictionary of the same form as described for
        retrieveAttrs from a tree structure
        :param node: Function Tree Node
        :param attrKey: Attribute name which be used to retrieve an attribute from a FunctionTree Node
                        That attribute will be the key to our returned dictionary (something like func or lib)
        :param attrVal: Attribute name which will be used to retrieve an attribute from a FunctionTree Node
                        That attribute will be used as the value for our returned dictionary (Like sampleTotal or execTime)
        :return: A Dictionary with a key:value pair of {attrKey:attrVal}
        """
        if not node.childFuncs:
            return {getattr(node, attrKey): getattr(node, attrVal)}
        attrDict = {}

        # Iterate through each child and merge the current node's dictionary and the child's dictionary
        # The current node's dictionary is attrDict,
        # and the child's dictionary is from the recursive call of getAttrsfromTree()
        for child in node.childFuncs.values():
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
    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: A dictionary of all functions merged by summing their sample counts, however they will be kept separate
    if their library differs.
    """
    # print("AAAA")
    def getFuncsSampsfromTree(tree):
        """
        Helper function for retrieving function samples while also maintaining library specificity
        :param tree: A function Tree Node
        :return: A dictionary of all functions merged by summing their sample counts as long as their corresponding
                 Library matches
        """
        if not tree.childFuncs:
            return {tree.lib: {tree.func: tree.sampleTotal}}
        funcDict = {}
        for child in tree.childFuncs.values():
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
    """
    This function retrieves a dictionary of all libraries merged by summing their sample totals however these libraries
    will not be merged if they are associated with different TIDs.
    Thus the resulting dictionary is {TID:Lib:sample count}

    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: a dictionary of all of the libraries and their samples stored with respect to TID = {TID:{Lib:sample count}}
    """

    def getLibsSampsfromTree(tree):
        """
        This is simply a helper function for retrieving the library information of nodes from a single function tree
        :param tree: A function tree node
        :return: A dictionary with a key:value pair of {Lib:sample count}. Sample counts are summed together if their
                 libraries match.
        """
        if not tree.childFuncs:
            # print(tree.lib, tree.sampleTotal)
            return {tree.lib: tree.sampleTotal}
        libDict = {}
        for child in tree.childFuncs.values():
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


def groupLibsandFuncs(TIDLibFuncDict):
    """
    This function will convert {TID:{Lib:{Func:sample/execution time}}} into {lib:{func:execution time/samples}}
    Essentially if we see a dictionary in the form of {lib:{func:exec time or samples}} regardless of thread id, we will
    merge those dictionaries with other ones that share the same library and function, by summing their values together.

    :param TIDLibFuncDict: A dictionary in the form of: {TID:{Lib:{Func:sample or execution time}}}
    :return: A dictionary in the form of: {lib:{func:execution time or samples}}
    """
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
    """
    Prints the first tree in our tidData dictionary which is simply the first TID then the first function tree found
    via the for loop.

    :param tidData: The dictionary in the form of {TID:{root function and library name: Function Tree}}
    :return: Nothing
    """
    for treeDict in tidData.values():
        printTree(treeDict)
        break
    return

# ===================================================================================
def main():
    print("in main")

    global useTimestamps

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
        useTimestamps = True

    loadData(tidData)
    # printFirstTreeDict(tidData)

    outDict = {} # The final output dictionary that will be written to chart_data.json

    # A dictionary to contain any removed data from outDict, if there are data in this dict, then we will write it to
    # removed_data.json
    removedDict = {}

    """
    Depending on what the user inputted for the timestamp prompt, 
    the script will process execution time or samples accordingly.
    
    One may edit the execution time processing section accordingly if they may want samples as well.
    Function trees node will have a sample count based on the number of times their function name was shown up in the data 
    at the call stack level the node is at.
    
    However, it is not guaranteed that execution time is available if using the sample processing section.
    
    This is because function trees are designed to always have sample counts, but not timestamps.
    This is just a consequence of being adaptable towards perf data.
    Scaler data should always have both sample values and timestamps.
    
    The output data format is designed so that there will be a top level key that will be represent the type of
    data stored with that key.
    
    For example: {tidlibs: value}
    tidlibs is a string that will key to another value which is most likely a dictionary that
    contains thread ids and all libraries that were called from a thread id as well as their execution times or samples.
    
    This makes it easy to request the data in charter. Charter may require modification in order
    to display the data. However that is fairly trivial.
    
    Perhaps one may do a slight refactoring of charter to handle any type of data. 
    And maybe implement a functionality
    that lets the user select data without modifying the script.
    """
    if useTimestamps:
        # print the first tree
        # printFirstTreeDict(tidData)

        # Retrieve the root execution times before redistribution
        # These execution times do not represent pure On-CPU execution.
        # This is explained in comments above this function's definition
        outDict["Threads"] = getThreadRootExe(tidData)

        redistributeExecTime(tidData)

        outDict["tidLibs"] = retrieveTIDLibs(tidData)

        # Retrieve the function execution times
        # At this point, the execution times have been redistributed
        # Therefore, the sum of these execution times should be the On-CPU execution times
        outDict["funcs"] = retrieveAttrs.__wrapped__(tidData, "func", "execTime")

        # Retrieve all of the execution times for functions and libraries relative to the thread they were called from
        outDict["tidlibsFuncs"] = retrieveTIDLibsFuncs.__wrapped__(tidData)

        # Groups all libraries and functions by ignoring thread association
        outDict["libsFuncs"] = groupLibsandFuncs(outDict["tidlibsFuncs"])

    else:
        # Sum all thread samples and store them with their respective threads
        outDict["Threads"] = getThreadSampTot(tidData)
        print(outDict["Threads"])

        # Perform sample redistribution
        redistributeSamples(tidData)
        # printFirstTreeDict(tidData)

        # Retrieving all library samples and store them with their respective libraries and in their respective threads
        outDict["tidLibs"] = retrieveTIDLibSamples.__wrapped__(tidData)
        # print("wow")
        print(outDict["tidLibs"])

        # Just to sum all of the library samples to see if they sum as expected
        # Should sum to the same amounts that is stored in outDict["Threads"]
        total = 0
        for index, tid in enumerate(outDict["tidLibs"].keys()):
            try:
                a = sum(outDict["tidLibs"][tid].values())
                total += a
                print(f"Tid: {tid}, sum: {a}")
            # Can happen with kernel attribution, some thread data may have nothing due to kernel root nodes being removed
            except AttributeError:
                print(f"TID: {tid}, sum:", outDict["tidLibs"][tid])
        print(total)

        # This will sum all of the values in each thread of outDict["LibSamps"] and report the sample totals for each
        # library irrespective of threads
        for libDict in outDict["tidLibs"].values():
            try:
                for lib in set(outDict["libs"]) | set(libDict):
                    outDict["libs"][lib] = outDict["libs"].get(lib, 0.0) + libDict.get(lib, 0.0)
            except KeyError:
                try:
                    outDict["libs"] = libDict.copy()
                # Can happen with kernel attribution, some thread data may have nothing due to kernel root nodes bring removed.
                except AttributeError:
                    continue

        print(outDict["libs"])
        print(outDict["tidLibs"])

        # This will grab the sample totals of all functions and store them with their respective threads and libraries
        outDict["tidlibsfuncs"] = retrieveFuncSamples.__wrapped__(tidData)

        pprint.pprint(retrieveTIDLibSamples(tidData))
        pprint.pprint(retrieveFuncSamples(tidData))
        # printFirstTreeDict(tidData)

    # Clean up the output dict of any None values.
    # None values can only appear if the thread data had no function trees in it.
    # This may occur due to kernel attribution.
    for key in list(outDict.keys()):
        if "tid" in key.lower() or "thread" in key.lower():
            if None in list(outDict[key].values()):
                for tid, val in list(outDict[key].items()):
                    if val is None:
                        if key in removedDict:
                            removedDict[key][tid] = outDict[key].pop(tid)
                        else:
                            removedDict[key] = {tid: outDict[key].pop(tid)}

    # If we removed anything from the output dictionary, we will write to the removed_data.json file so that
    # users can see what was removed.
    if removedDict:
        """
        NOTE:
        The file name is the same regardless of data. Therefore, when you use converter on some data.
        Make sure to save the removed data file somewhere else. Otherwise, subsequent runs of converter will overwrite
        your previous removed data. 
        
        One can edit charter or this script to move the removed data file to some directory automatically.
        """
        print("Removed null data... Writing it to removed_data.json")
        with open("removed_data.json", 'w') as removed_file:
            json.dump(removedDict, removed_file, indent=4)

    # Dump our output data into the chart data json so we can chart stuff
    with open("chart_data.json", 'w') as chart_file:
        json.dump(outDict, chart_file, indent=4)


if __name__ == "__main__":
    main()