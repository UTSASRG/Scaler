# By: John Diep
# Data Aggregator V2
import tkinter as tk
from tkinter import filedialog
import sys
import json
'''

Input Data Format:
command-pid/tid;(bottom of stack) func1 [library] or [library] timestamp start timestamp end; func2 [library] or [library] timestamp start timestamp end;...;(Top of stack) funcn [library] or [library] timestamp start timestamp end sample count

Output Data Structure:
{tid: {root function name: Function Tree}}

'''
# This will be set to true if the user acknowledges the use of timing information
# Otherwise, this remains false
# When true, I will check for data validation on timestamps to ensure that the user has valid timing data and then
# the timestamps will be set for each function node
# If false, then I will forego the timing information and completely remove it from the final data structure
# To save on data
useTimestamps = False

# TODO Consider samples with call stacks that are a "substring" of another call stack
# EXAMPLE: [memcached];__libc_start_main [libc-2.31.so];[memcached];event_base_loop [libevent-2.1.so.7.0.0];[libevent-2.1.so.7.0.0]
#          [memcached];__libc_start_main [libc-2.31.so];[memcached];event_base_loop [libevent-2.1.so.7.0.0];[libevent-2.1.so.7.0.0];epoll_wait [libc-2.31.so];entry_SYSCALL_64_after_hwframe_[k];do_syscall_64_[k];__x64_sys_epoll_wait_[k];do_epoll_wait_[k];ktime_get_ts64_[k]
class FunctionTree:
    __slots__ = ['root', 'func', 'lib', 'timestamps', 'sampleTotal', 'childFuncs']
    def __init__(self, sampleCnt, lib, func='Unknown', timestamps=()):
        # self.root = True
        self.func = func
        self.lib = lib
        # If the user requests to use timing information, then I will expect that every function
        # has appropriate timestamp information, if not then we will exit and report the error
        if useTimestamps:
            if not timestamps:
                print("ERROR: No Timestamps available, exiting...")
                sys.exit()
            else:
                try:
                    float(timestamps[0])
                    float(timestamps[1])
                except ValueError:
                    print("ERROR: Timestamps are not floats, exiting...")
                    sys.exit()
            self.timestamps = timestamps
        self.sampleTotal = sampleCnt
        self.childFuncs = []

    # Adds a node to the child list if it does not exist, otherwise if we find the function and library in our child list
    # we will update its sample count and timestamps (effectively merging the samples) then return the child
    # If we made a new node, then we will also return this new node
    def addNode(self, sampleCnt, lib, func="Unknown", timestamps=()):
        # cnt = 0
        # If the child function list has items in it, then we will search for a node with the current function and library
        # If a node is found, then we will update its values with the current sample count and the timestamps
        # If a node is not found or the list is empty then we know we need to create a new child node
        if self.childFuncs:
            for child in self.childFuncs:
                if child.func == func and child.lib == lib:
                    # if cnt > 1:
                    #    print("Error: Multiple of one function detected in child nodes, exiting...")
                    #    sys.exit()
                    child.sampleTotal += sampleCnt
                    if useTimestamps:
                        if not timestamps:
                            print("Error: No Timestamps Available, exiting...")
                            sys.exit()
                        child.updateTimestamps(timestamps)
                    return child
        newNode = FunctionTree(sampleCnt, lib, func, timestamps)
        self.childFuncs.append(newNode)
        return newNode
            # cnt += 1

    def updateTimestamps(self, timestamps):
        try:
            float(timestamps[0])
            float(timestamps[1])
        except ValueError:
            print("ERROR: Timestamps are not floats, exiting...")
            sys.exit()
        if timestamps[0] < self.timestamps[0]:
            print("Error: Impossible data, Start timestamp happened earlier than child's start timestamp, exiting...")
            sys.exit()
        # If data passes the previous test, then we know that the timestamps always occur after the start of execution
        # of the current child function
        # This makes sense since data tends to happen sequentially or in parallel, but there is always a
        # forward (time) moving causal relationship with the data, thus data appearing in the past makes no sense
        # unless data is out of order, but I do not know why one would want out of order data

        # TODO Figure out how to deal with timestamp merge; sum durations? take earliest timestamp and latest timestamp?

        # The Goal of this method is to merge samples, to do this, I will simply save the earliest timestamp recorded for
        # the current node's function and I will save the latest timestamp
        # The start timestamp of the timestamps parameter passed in will either be the same or greater than
        # the current node's start timestamp, there is no need to update the start timestamp as the new timestamp
        # will never be earlier than the current function's timestamp

        # Swap the current end timestamp with the new one if it is later than the current timestamp
        if timestamps[1] > self.timestamps[1]:
            self.timestamps[1] = timestamps[1]
        return

    def __str__(self):
        # print("Noot")
        return f"Function: {self.func}; Lib: {self.lib}; Sample Total: {self.sampleTotal}; Child Functions: {self.childFuncs}"

    def __retrieveAttrs(self):
        if useTimestamps:
            return {"func":self.func, "lib":self.lib, "sampleTotal":self.sampleTotal, "timestamps":self.timestamps, "childFuncs":[]}
        else:
            return {"func":self.func, "lib":self.lib, "sampleTotal":self.sampleTotal, "childFuncs":[]}

    def serialize(self):
        tempList = []
        if len(self.childFuncs) <= 0:
            return self.__retrieveAttrs()
        for childFunc in self.childFuncs:
            tempList.append(childFunc.serialize())
        aDict = self.__retrieveAttrs()
        aDict["childFuncs"] = tempList
        return aDict


#If the stack collapsing script was called with additional annotation for kernel, inlined and jitted functions,
#then I will declare the function's library as kernel, inlined, or jitted respectively and remove the suffix
#If a normal input of function [library] or [library] was passed in then, simply return 0 to indicate nothing was done
def checkSuffix(func):
    suffix = {"_[k]": "Kernel", "_[i]": "Inlined", "_[j]": "Jit"}
    if any(suf in func for suf in suffix.keys()):
        aList = func.split()
        funcLen = len(aList[0])
        # print(func[0:-4], func, func[funcLen-4:], funcLen)
        moduleName = suffix.get(aList[0][funcLen-4:], "error")
        if moduleName == "error":
            print("Error: Failed to find module name for kernel, inlined, or jitted function")
            sys.exit() #Error occurred so print and exit
        if len(aList) > 1:
            return [aList[0][:-4], moduleName, aList[1], aList[2]]  # Remove the _[?] section from the function name, aList[1] and [2] contain timestamps
        else:
            return [aList[0][:-4], moduleName]
    return 0

def parseFunc(funcLine, index):
    #Check for _[k], _[i], and _[j]
    tempList = checkSuffix(funcLine[index])

    # If the suffixes were not detected then grab the temporary function list from the actual line list
    if tempList == 0:
        tempList = funcLine[index].split()

    # For data validation
    tempLen = len(tempList)

    # Check for the various possible inputs
    if tempLen > 4 or tempLen < 1:
        print("Error: Impossible Input, exiting...")
        sys.exit()
    elif tempLen == 4 or tempLen == 2:
        # If we have all possible inputs, function, library and timestamps
        if tempLen == 4:
            try:
                float(tempList[2])
                float(tempList[3])
            except ValueError:
                print("Error: Impossible input, exiting...")
                sys.exit()
            return (1, tempList[0], tempList[1], (tempList[2], tempList[3]))
        # Otherwise just function and library
        else:
            return (2, tempList[0], tempList[1])
    else:
        # If we just have the library and timestamps
        if tempLen == 3:
            try:
                float(tempList[1])
                float(tempList[2])
            except ValueError:
                print("Error: Impossible Input, exiting...")
                sys.exit()
            return (3, tempList[0], (tempList[1], tempList[2]))
        # If we just have the library
        else:
            return (4, tempList[0])
    # return "ERROR"

def constructFuncBranch(root, sampleNum, funcLine, index):
    # print(f"Poop {index}")
    # If our index exceeds the length of the line list, then we must end the recursion as we cannot make more nodes
    if index >= len(funcLine):
        return
    # Grab the data from the current function
    funcResult = parseFunc(funcLine, index)

    # Check all possible inputs, there are 4 possible inputs
    # We will add a node if the current function has not had a node created at the current stack depth and return it
    # If a node is found at the current stack depth created for the current function then, we will simply retrieve that node
    # This node will be used to recursively construct the branch in our function tree
    # A branch in the function tree is one unique call stack seen in the samples/traces

    if funcResult[0] == 1:
        # funcResult = (1, func, lib, time start, time end)
        root = root.addNode(sampleNum, funcResult[2], func=funcResult[1], timestamps=(funcResult[3], funcResult[4]))
    elif funcResult[0] == 2:
        # funcResult = (2, func, lib)
        root = root.addNode(sampleNum, funcResult[2], func=funcResult[1])
    elif funcResult[0] == 3:
        # funcResult = (3, lib, time start, time end)
        root = root.addNode(sampleNum, funcResult[1], timestamps=(funcResult[2], funcResult[3]))
    elif funcResult[0] == 4:
        # funcResult = (4, lib)
        root = root.addNode(sampleNum, funcResult[1])
    return constructFuncBranch(root, sampleNum, funcLine, index + 1)

# This is a helper function to create a new function tree if one has not been created for the first function seen in the
# current line. If the first function has had a function tree created for it then we will return that node, otherwise we
# have created a new node and will return that new node
def createNewFuncTree(tid, treeDict,  sampleNum, funcLine):
    funcResult = parseFunc(funcLine, 0)
    if funcResult[0] == 1:
        # funcResult = (1, func, lib, time start, time end)
        rootNode = treeDict.setdefault(' '.join(funcResult[1:3]), FunctionTree(sampleNum, funcResult[2], funcResult[1], (funcResult[3], funcResult[4])))
    elif funcResult[0] == 2:
        # funcResult = (2, func, lib)
        rootNode = treeDict.setdefault(' '.join(funcResult[1:]), FunctionTree(sampleNum, lib=funcResult[2], func=funcResult[1]))
    elif funcResult[0] == 3:
        # funcResult = (3, lib, time start, time end)
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(sampleNum, lib=funcResult[1], timestamps=(funcResult[2], funcResult[3])))
    else:
        # funcResult = (4, lib)
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(sampleNum, lib=funcResult[1]))
    return rootNode

def printBranch(nodeList):
    if not nodeList:
        return
    for node in nodeList:
        print(node)
        return printBranch(node.childFuncs)


def printTree(treeList):
    num = 1
    for tree in treeList.values():
        print(f"TREE {num}:", tree)
        printBranch(tree.childFuncs)
        num += 1
    return

# TODO NEED TO CONSIDER RECORDING TOTAL THREAD SAMPLES AND TOTAL LIBRARY SAMPLES
def main():
    tidData = {}
    # tidTree = FunctionTree()
    # Ask user for an input file, they can refuse it and the aggregator will use a default file path set to finalFold.folded in the repo
    root = tk.Tk()
    root.withdraw()
    fileName = filedialog.askopenfilename()

    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        global useTimestamps
        useTimestamps = True

    if fileName == '':
        # If no file name then just default to opening a file in the repo
        # print(True)
        fileName = "C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/finalFold.folded"
    currentTID = ''
    # print("Wow")
    with open(fileName) as fold:
        for line in fold:
            lineList = line.split(';') # Line is semi-colon separated, thus we split using ;

            '''
            If the tid has not been registered into the tid data dictionary then create an entry with an empty dictionary
            If the tid has been registered, then we will retrieve the dictionary of function trees associated
            with the tid
            '''
            tid = lineList.pop(0)
            treeDict = tidData.setdefault(tid, {})
            # print(treeDict)
            # treeDict.setdefault()

            # This section will clean up the line list of the sample number that way I do not have to deal with the edge case of parsing the sample number
            # Every element in the line list will be the same format as a result except for the first element
            # The first element is the command-pid/tid format
            lastEleList = lineList[-1].split() # The last element split by white space
            sampleNum = int(lastEleList[-1]) # Retrieve the line's sample number
            lineList[-1] = ' '.join(lastEleList[:-1])  # Strip the last element of the line list of the sample number

            # Grab the function tree with the first function in the sample as the root
            rootNode = createNewFuncTree(tid, treeDict, sampleNum, lineList)

            constructFuncBranch(rootNode, sampleNum, lineList, 1)
            '''
            if currentTID != tid:
                if currentTID == '':
                    currentTID = tid
                else:
                    printTree(tidData[currentTID])
                    break
            '''
    with open("perfMemcachedData_V2.json", 'w') as j_file:
        j_file.write(json.dumps(tidData, default=lambda tree: tree.serialize(),indent=4))

if __name__ == "__main__":
    main()