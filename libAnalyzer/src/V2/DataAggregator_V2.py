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
{} = dictionary
Function Tree = an instance of the Function Tree Class
root function name = the function and library associated with the root node of the function tree it keys to

Function Tree is a generic tree data structure where a node represents a single function call on a call stack

Each function tree node consists these basic pieces of information:
    - Function name
    - Library Name
    - Total Samples recorded for this function
    - Timestamps (If available)
    - List of child nodes
      - These child nodes represent the next function that was called in the call stack
      - There may be an arbitrary amount of child nodes because we merge call stacks with the same call sequence and only
      - diverge them once we detect a function that is different from known child functions
      - An example of this is seen below above the Function Tree definition

I use the root function name to key to a specific function tree 
because one may know the first function that was called for a call stack
And thus they can find any call stack that may have resulted from that first function call. 

If I implemented just a list of function trees, the worst case search would be O(n) when trying to find the 
root function given a function name and library
With the dictionary, the worst case search is O(1) since keying a dictionary uses a hash map

'''
# This will be set to true if the user acknowledges the use of timing information
# Otherwise, this remains false
# When true, I will check for data validation on timestamps to ensure that the user has valid timing data and then
# the timestamps will be set for each function node
# If false, then I will forego the timing information and completely remove it from the final data structure
# To save on data
useTimestamps = False
attributeInline = False

class FunctionTree:
    __slots__ = ['func', 'lib', 'timestamps', 'sampleTotal', 'childFuncs', 'execTime']
    def __init__(self, sampleCnt, lib, func='Unknown', timestamps=[], useTime=False):
        # self.root = True
        """
        Initializes a node of the Function Tree. Makes use of __slots__ for more efficiency
        (faster access times, and more efficient memory usage)
        :param sampleCnt: Initial sample value attributed to this node's function/library
        :param lib: Library name for the node
        :param func: Function name for the node, Default value is "Unknown" for when we don't know the function name
        :param timestamps: Initial list of timestamps to be attributed to the node's function
        :param useTime: Boolean for when we reconstruct the tree but do not have access to the useTimestamps value in
                        this script's scope. Defaults to false, set to true during reconstruction to attribute time
        """
        self.func = func
        self.lib = lib
        # If the user requests to use timing information, then I will expect that every function
        # has appropriate timestamp information, if not then we will exit and report the error
        if useTimestamps or useTime:
            if not timestamps:
                raise Exception("ERROR: No Timestamps available, exiting...")
            else:
                self.timestamps = []
                self.execTime = 0
                try:
                    for timeTup in timestamps:
                        # I do not know the units of the timestamps before hand,
                        # so may need to specify time units afterwards
                        # or settle on one time unit to use
                        self.addTimestamps(timeTup)
                except ValueError:
                    raise Exception("ERROR: Timestamps are not floats, exiting...")
        self.sampleTotal = sampleCnt
        # self.childFuncs = []
        self.childFuncs = {}


    def addNode(self, sampleCnt, lib, func="Unknown", timestamps=()):
        """
        Adds a node to the child list if it does not exist,
        otherwise if we find the function and library in our child list
        we will update its sample count and timestamps (effectively merging the samples) then return the child
        If we made a new node, then we will also return this new node
        :param sampleCnt: Initial Sample value to attribute to new node or to add onto an existing node's sample total
        :param lib: Library name to attribute to new node or to check if child node exists
        :param func: Function name to attribute to new node or to check if child node exists
        :param timestamps: Timestamp tuple to add to an existing node's timestamp list or to create a new node
        :return: a child node with lib and func if it exists, or the new node we created
        """
        # cnt = 0
        # If the child function list has items in it, then we will search for a node with the current function and library
        # If a node is found, then we will update its values with the current sample count and the timestamps
        # If a node is not found or the list is empty then we know we need to create a new child node
        if self.childFuncs:
            '''for child in self.childFuncs:
                if child.func == func and child.lib == lib:
                    # if cnt > 1:
                    #    print("Error: Multiple of one function detected in child nodes, exiting...")
                    #    sys.exit()
                    child.sampleTotal += sampleCnt
                    if useTimestamps:
                        if not timestamps:
                            raise Exception("Error: No Timestamps Available, exiting...")
                        child.addTimestamps(timestamps)
                    return child'''
            child = self.childFuncs.get(f"{func} {lib}", None)
            if child is not None:
                child.sampleTotal += sampleCnt
                if useTimestamps:
                    if not timestamps:
                        raise Exception("Error: No Timestamps Available, exiting...")
                    child.addTimestamps(timestamps)
                return child
        newNode = FunctionTree(sampleCnt, lib, func, [timestamps])
        self.childFuncs[f"{func} {lib}"] = newNode
        return newNode
            # cnt += 1

    def addTimestamps(self, timestamps):
        try:
            timestamps = (float(timestamps[0]), float(timestamps[1]))
        except ValueError:
            raise Exception("ERROR: Timestamps are not floats, exiting...")
        # print(timestamps)
        self.timestamps.append(timestamps)

    # Method for updating the timestamps of the node it was called by merging timestamps
    def updateTimestamps(self):
        """
        These are the various cases that have to be considered regarding this function
            example timestamp tuple: |---------|
                                    start     end


            |-------|   |-------|

            2 timestamps tuples, but they are not overlapping

            |-------|-------|
            2 timestamp tuples, but they occur right after another (first end timestamp == second start timestamp)
        =====================================================================================================
        General idea for merging these tuples:

        Assumptions:
           - List is sorted in ascending order by start timestamp

        As we find tuples that match this form:
        |--------|-------|

        we will save the earliest start and the latest end timestamp
        Then replace all of the tuples that were merged with the new merged tuple which contains the earliest start
        timestamp and the latest end timestamp

        :return: nothing
        """

        # Given the nature of profiling data, and that these nodes are stored relative to a specific thread id
        # Timestamps are added in, in sequential order, meaning that the self.timestamps list
        # should already be sorted in ascending order based on the start timestamp
        # If somehow the list is not sorted, then that indicates a problem with the profiling data.
        if self.timestamps != sorted(self.timestamps):
            raise Exception("Data Error: Timestamps are not already sorted.")

        # (currentStart, currentEnd) will be the new merged timestamp tuple
        currentStart = 0
        currentEnd = 0

        # tupleCount will be the offset from the start index as well as the count of the number of tuples being merged
        tupleCount = 0

        # As we merge tuples we will append them to the list, any tuples that were not merged will simply be appended
        # to the list as well.
        newList = []

        for timestampTup in self.timestamps:
            # If currentEnd has been reset, then assign the current tuple.
            if currentEnd == 0:
                currentStart, currentEnd = timestampTup[0], timestampTup[1]
                tupleCount += 1
                continue

            # If the next tuple's start timestamp = the previous tuple's end timestamp, then we will extend our current
            # merged tuple with the end timestamp of the current tuple
            if currentEnd == timestampTup[0]:
                currentEnd = timestampTup[1]
                if timestampTup == self.timestamps[-1]:
                    newList.append((currentStart, currentEnd))
            # Otherwise, we need to save the current merged tuple along with the indices of the tuples we will replace
            else:
                # Once we detect that the previous end timestamp does not match the current start timestamp
                # We will add the merged timestamps to the new list then reassign the current start timestamp
                # and end timestamp to the merging timestamp variables
                newList.append((currentStart, currentEnd))
                currentStart, currentEnd = timestampTup[0], timestampTup[1]
                tupleCount = 0
                if timestampTup == self.timestamps[-1]:
                    newList.append((currentStart, currentEnd))

            tupleCount += 1

        if not newList and tupleCount == len(self.timestamps):
            newList.append((currentStart, currentEnd))
            self.execTime += currentEnd - currentStart
        else:
            for timeTup in newList:
                self.execTime += timeTup[1] - timeTup[0]

        if not newList and self.timestamps:
            print("ERROR: newList is empty but timestamps list is not, exiting...")
            sys.exit()
        # The timestamp list will be replaced with the new list that contains the merged tuples
        self.timestamps = newList

        return


    def __str__(self):
        """
        Special method for when print is called on a node, simply prints all of the attributes
        :return: A string with all of the attribute names and the values
        """
        # print("Noot")
        retStr = ""
        for att in dir(self):
            try:
                if not att.startswith('__') and not callable(getattr(self,att)):
                    retStr = f"{att}: {getattr(self, att)} " + retStr
            except AttributeError:
                continue
        return retStr

    def __retrieveAttrs(self):
        """
        This is a private method to retrieve all of the attributes (aside from the child function list)
        and store them into a dictionary which is used for serialization
        :return: A dictionary with all of the attributes and their value except for child functions, which an empty list
        is returned instead
        """
        if useTimestamps:
            return {"func":self.func, "lib":self.lib, "sampleTotal":self.sampleTotal, "timestamps":self.timestamps, "execTime":self.execTime, "childFuncs":[]}
        else:
            return {"func":self.func, "lib":self.lib, "sampleTotal":self.sampleTotal, "childFuncs":[]}


    def serialize(self):
        """
        This method will convert the current function tree into a serializable json format
        This method assumes that serialize was called from the root node of the function tree
        If called at any depth aside from root level, information will be lost about nodes prior the the node serialize
        was called from
        :return: A dictionary representing the entire tree that has the node serialize was called from as the root
                 May be a sub-tree of the original function tree or the whole function tree if called from the root node
                 Generally we want to call this from the root node to serialize the whole tree
        """
        tempList = []
        if len(self.childFuncs) <= 0:
            return self.__retrieveAttrs()
        for childFunc in self.childFuncs.values():
            tempList.append(childFunc.serialize())
        aDict = self.__retrieveAttrs()
        aDict["childFuncs"] = tempList
        return aDict

    def __setExecTime(self):
        """
        For use with tree reconstruction, when we have timestamps, we will recalculate the execution time from
        the timestamps attributed to the node
        :return: Node
        """
        for timeTup in self.timestamps:
            self.execTime += timeTup[1] - timeTup[0]
        return

    @staticmethod
    def reconstructNodefromDict(j_Dict, useTime=False):
        """
        Static method of the Function Tree Class
        Allows for reconstruction of a function tree given a dictionary that was created from the Serialize method
        of this class or of the same format.
        :param j_Dict: A dictionary generated by serialize() of FunctionTree class or a dictionary of same format
        :param useTime: Boolean for if we need to attribute time, default value is False, when set to True, we
                        attribute timestamps and recalculate execution time from the timestamps
        :return: A new node we created, this is a recursive function so we use the node to construct more nodes
                 if the dictionary indicates there are child functions nodes to be created
        """
        try:
            if useTime:
                node = FunctionTree(sampleCnt=j_Dict["sampleTotal"], lib=j_Dict["lib"], func=j_Dict["func"], timestamps=j_Dict["timestamps"], useTime=useTime)
                # node.__setExecTime()
                node.execTime = j_Dict["execTime"]
            else:
                node = FunctionTree(sampleCnt=j_Dict["sampleTotal"], lib=j_Dict["lib"], func=j_Dict["func"])
        except KeyError:
            raise Exception("Data Error: Data missing or data corrupted. ")

        if len(j_Dict["childFuncs"]) <= 0:
            return node
        for child in j_Dict["childFuncs"]:
            #node.childFuncs.append(FunctionTree.reconstructNodefromDict(child, useTime))
            func = child["func"]
            lib = child["lib"]
            if node.childFuncs.get(f"{func} {lib}", None) is None:
                node.childFuncs[f"{func} {lib}"] = FunctionTree.reconstructNodefromDict(child, useTime)
            else:
                print(func, lib, node.childFuncs[f"{func} {lib}"])
                raise Exception("Data Error: Duplicate node detected.")
        return node

    def __get_leaf_nodes(self, leaves):
        """
        Private helper method to retrieve the leaf nodes
        :param leaves: A list that will contain the leaf nodes
        :return: Nothing
        """
        if not self.childFuncs:
            nodeDict = self.__retrieveAttrs()
            del nodeDict['childFuncs']
            leaves.append(nodeDict)
            return
        for child in self.childFuncs.values():
            child.__get_leaf_nodes(leaves)

    def retrieveLeaves(self):
        """
        Method for retrieving the leaf nodes
        :return: A list of the leaf nodes
        """
        leaves = []
        self.__get_leaf_nodes(leaves)
        return leaves

# ============================================ Data Cleanup and Organization ===================================


def checkSuffix(func):
    """
    If the stack collapsing script was called with additional annotation for kernel, inlined and jitted functions,
    then I will declare the function's library as kernel, inlined, or jitted respectively and remove the suffix
    If a normal input of function [library] or [library] was passed in then, simply return 0 to indicate nothing was done
    :param func: A string that shows a function name
    :return: A list of the function name stripped of the suffix and the library corresponding to the suffix OR
             0 if we did not detect the suffix.
    """
    suffix = {"_[k]": "Kernel", "_[i]": "Inlined", "_[j]": "Jit"}
    if any(suf in func for suf in suffix.keys()):
        funcLen = len(func)
        moduleName = suffix.get(func[funcLen-4:], "error")

        if moduleName == "error":
            raise Exception("Error: Failed to find module name for kernel, inlined, or jitted function")

        return [func[:-4], moduleName]
    return 0

# This will take a function string from the raw data and retrieve any information possible
# The possible information is: function name, library name, timestamps and sample count
def parseFunc(funcLine, index):
    """
    This will parse a single function sample of this form: func [library] or [library] timestamp start timestamp end
    There are 4 possible input forms that will be handled
    Case 1: func [lib]
    Case 2: [lib]
    Case 3: func [lib] timestamp start timestamp end
    Case 4: [lib] timestamp start timestamp end

    One special case:
    func with _[k] or _[i] or _[j] tacked on
    We will strip the function of this suffix via checkSuffix()

    This will lead to an output format of Case 1 or Case 3

    :param funcLine: A list of functions libraries and timestamps each element in the form described above as a string.
    :param index: Index of the function to parse
    :return: A tuple containing the function name, library name, and timestamps extracted from the function string and
             and ID value of 1 2 3 or 4 which indicates the type of data available in the tuple
             (ID, data1, data2, data3)
             (1, func, lib, (time start, time end))
             (2, func, lib)
             (3, lib, (time start, time end))
             (4, lib)
    """

    tempList = funcLine[index].split()
    # Check for _[k], _[i], and _[j]
    retVal = checkSuffix(tempList[0])

    # If the suffixes were not detected then grab the temporary function list from the actual line list
    if retVal:
        # print("before", tempList)
        if len(tempList) > 1:
            tempList = retVal + tempList[1:]
        else:
            tempList = retVal
        # print("after", tempList)
    # For data validation
    tempLen = len(tempList)

    # Check for the various possible inputs
    if tempLen > 4 or tempLen < 1:
        raise Exception("Error: Impossible Input, exiting...")
    elif tempLen == 4:
        # If we have all possible inputs, function, library and timestamps
        try:
            float(tempList[2])
            float(tempList[3])
        except ValueError:
            raise Exception("Error: Impossible Input, exiting...")
        # if tempList[2] == "2130297.050642":
        #    print("End of file")
        return 1, tempList[0], tempList[1], (tempList[2], tempList[3])
    elif tempLen == 2:
        # Otherwise just function and library
        return 2, tempList[0], tempList[1]
    elif tempLen == 3:
        # If we just have the library and timestamps
        try:
            float(tempList[1])
            float(tempList[2])
        except ValueError:
            raise Exception("Error: Impossible Input, exiting...")
        return 3, tempList[0], (tempList[1], tempList[2])
    else:
        # If we just have the library
        return 4, tempList[0]
    # return "ERROR"

# =================================================================================================

# ======================================= Tree Construction Functions =============================

def constructFuncBranch(root, sampleNum, funcLine, index):
    """
    Each line from the data represents a single branch in our function trees (due to the call stack),
    thus given a line from the data, we will construct this branch in our function tree
    This is done recursively by traversing down our function tree creating new nodes as needed until we reach
    the end of the call stack

    :param root: A Function Tree Node
    :param sampleNum: A sample value
    :param funcLine: The Function sample list
    :param index: Index to use with the funcLine List
    :return: Nothing
    """
    # If our index exceeds the length of the line list, then we must end the recursion as we cannot make more nodes
    if index >= len(funcLine):
        return
    # Grab the data from the current function
    funcResult = list(parseFunc(funcLine, index))

    # Attribute all inlined functions to the last known library
    if attributeInline:
        # Last known library will propagate through consecutive inlined functions,
        # Thus the parent node will have the last known library
        # Which means we can check for this by checking if the library is set to inlined because using this method
        # the only way for the final result to have inlined as the library, would have to be that the first level
        # of the call stack is inlined
        if "inline" not in root.lib:
            if funcResult[0] <= 2:
                # Only attribute the last known library as the library of the new node if the new node's library has
                # inline in the string
                if "inline" in funcResult[2]:
                    funcResult[2] = root.lib
            else:
                if "inline" in funcResult[1]:
                    funcResult[1] = root.lib


    # We will add a node if the current function has not had a node created at the current stack depth and return it
    # If a node is found at the current stack depth created for the current function then
    # we will simply retrieve that node
    # This node will be used to recursively construct the branch in our function tree

    if funcResult[0] == 1:
        # funcResult = (1, func, lib, (time start, time end))
        root = root.addNode(sampleNum, funcResult[2], func=funcResult[1], timestamps=funcResult[3])
    elif funcResult[0] == 2:
        # funcResult = (2, func, lib)
        root = root.addNode(sampleNum, funcResult[2], func=funcResult[1])
    elif funcResult[0] == 3:
        # funcResult = (3, lib, (time start, time end))
        root = root.addNode(sampleNum, funcResult[1], timestamps=funcResult[2])
    elif funcResult[0] == 4:
        # funcResult = (4, lib)
        root = root.addNode(sampleNum, funcResult[1])
    return constructFuncBranch(root, sampleNum, funcLine, index + 1)

def createNewFuncTree(treeDict, sampleNum, funcLine):
    """
    This is a helper function to create a new function tree if one has not been created for the first function seen in the
    current line. If the first function has had a function tree created for it then we will return that node, otherwise we
    have created a new node and will return that new node

    :param treeDict: A dictionary with key:value pairs of root function name: root node
    :param sampleNum: Sample value
    :param funcLine: Function sample list
    :return: The newly created root node or if it has been created before, a retrieved root node
    """

    funcResult = parseFunc(funcLine, 0)
    if funcResult[0] == 1 and useTimestamps:
        # funcResult = (1, func, lib, (time start, time end))
        # print(funcResult)
        try:
            time = (float(funcResult[3][0]), float(funcResult[3][1]))
        except ValueError:
            raise Exception("Error: Timestamps are not float, exiting...")
        rootNode = treeDict.setdefault(' '.join(funcResult[1:3]), FunctionTree(0, lib=funcResult[2], func=funcResult[1], timestamps=[funcResult[3]]))

        if time not in rootNode.timestamps:
            rootNode.addTimestamps(time)

    elif funcResult[0] == 2 or (funcResult == 1 and not useTimestamps):
        # funcResult = (2, func, lib) or funcResult = (1, func, lib, (time start, time end)) while useTimestamps is false
        rootNode = treeDict.setdefault(' '.join(funcResult[1:]), FunctionTree(0, lib=funcResult[2], func=funcResult[1]))
    elif funcResult[0] == 3 and useTimestamps:
        # funcResult = (3, lib, (time start, time end))
        try:
            time = (float(funcResult[2][0]), float(funcResult[2][1]))
        except ValueError:
            raise Exception("Error: Timestamps are not float, exiting...")
        # print(funcResult)
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(0, lib=funcResult[1], timestamps=[funcResult[2]]))
        if time not in rootNode.timestamps:
            rootNode.addTimestamps(time)

    else:
        # funcResult = (4, lib) or funcResult = (3, lib, (time start, time end)) while useTimestamps is false
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(0, lib=funcResult[1]))
    rootNode.sampleTotal += sampleNum
    return rootNode

def mergeTimestamps(tidData):
    """
    Function to be called after all the data has been processed to create our function trees
    Merges the timestamps stored in each node's timestamp list
    :param tidData: The final dictionary with all of the function trees
    :return: Nothing
    """
    def traverseTreeandUpdate(node):
        if not node.childFuncs:
            node.updateTimestamps()
            return

        for child in node.childFuncs.values():
            traverseTreeandUpdate(child)

        node.updateTimestamps()
        return

    for treeDict in tidData.values():
        for tree in treeDict.values():
            traverseTreeandUpdate(tree)
    return

# =========================================================================================

# ======================================= Debugging Functions =============================

def printBranch(nodeList, printrootchild, branchNum=0):
    """
    Prints the branches of a node
    :param nodeList: A list of nodes, typically the list of child nodes
    :param printrootchild: Boolean for indicating when we're printing the root node
    :param branchNum: Current branch number from the parent node of the nodeList
    :return: Nothing
    """
    if not nodeList:
        return
    for node in nodeList:
        if printrootchild:
            print("ROOT BRANCH: ")
        else:
            print(f"BRANCH #{branchNum}: ")
        print(node)
        printBranch(node.childFuncs.values(), False, branchNum+1)
    return

# Prints all of the function trees in a given dictionary of function trees
def printTree(treeList):
    """
    Prints all of the trees stored in the tree List
    :param treeList: A list of function trees, these would be the root nodes
    :return: Nothing
    """
    num = 1
    for tree in treeList.values():
        print(f"TREE {num}:\n", tree)
        printBranch(tree.childFuncs.values(), True)
        num += 1
    return

# ============================================================================================

def main():
    tidData = {}
    # tidTree = FunctionTree()
    # Ask user for an input file, they can refuse it and the aggregator will use a default file path set to finalFold.folded in the repo
    root = tk.Tk()
    root.withdraw()
    fileName = filedialog.askopenfilename()

    # If an input file was selected, then we will ask them for an output file. They can opt to stop the program
    # By directly closing the file dialog twice in a row.
    # Otherwise if an input file is not selected, we will use some default files
    '''
    TO USE MY FAKE DATA:
       Select tests/Test_Cases/aggregator_Test1.folded as input
       Then select src/V2/fakeData.json as the output file
       Then enter "y" for using timestamps
       Then in Converter, just select fakeData.json as the input and say you want to use the timestamp data by entering
       "y"
    '''
    if fileName == '':
        # If no file name then just default to opening a file in the repo
        # print(True)
        fileName = "C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/finalFold.folded"
        outFileName = "perfMemcachedData_V2.json"
    else:
        stopBool = False
        while True:
            outFileName = filedialog.askopenfilename()
            if outFileName == '':
                # File dialog was closed, set stopBool to true. If file dialog is closed again, then we exit the program
                if not stopBool:
                    print("If you want to stop. Then close the file dialog again.")
                    stopBool = True
                else:
                    return
            else:
                break
    # We will handle the data differently depending on if the user wants to use the timing data
    # If "y" is entered, then we will use the time stamp info and sample data, if not then we will use sample data by default
    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        # print(timestampInput)
        global useTimestamps
        useTimestamps = True

    inlinedInput = input("Attribute Inlined functions to last known library? y/n Default is n: ")
    if inlinedInput == "y":
        global attributeInline
        print(attributeInline)
        attributeInline = True
    # currentTID = ''
    # print("Wow")
    with open(fileName) as fold:
        # a = 0
        lineNum = 1
        for line in fold:
            # print(lineNum)
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
            # sampleNum *= 1/997
            # sampleNum /= len(lineList)
            lineList[-1] = ' '.join(lastEleList[:-1])  # Strip the last element of the line list of the sample number

            # Grab the function tree with the first function in the sample as the root
            rootNode = createNewFuncTree(treeDict, sampleNum, lineList)

            constructFuncBranch(rootNode, sampleNum, lineList, 1)
            # if a > 20:
            #    break
            '''
            if currentTID != tid:
                if currentTID == '':
                    currentTID = tid
                else:
                    for treeDict in tidData.values():
                        printTree(treeDict)
                    break
            '''
            # a += 1
            # print(lineList, sampleNum)
            # if lineNum > 100:
            #    break
            #else:
            #    print(lineNum)
            lineNum += 1
    if not tidData:
        print("ERROR: Tid Data Dict empty, exiting...")
        sys.exit()

    # for treeDict in tidData.values():
    #    printTree(treeDict)
    if useTimestamps:
        mergeTimestamps(tidData)

    # print("After Merging")
    # for treeDict in tidData.values():
    #    printTree(treeDict)
    with open(outFileName, 'w') as j_file:
        j_file.write(json.dumps(tidData, default=lambda tree: tree.serialize()))

if __name__ == "__main__":
    main()