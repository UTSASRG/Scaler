# By: John Diep
# Data Aggregator V2
import tkinter as tk
from tkinter import filedialog
import sys
import json

'''
Convert perf data to tracing datastructure

Input: Perf sampling data processed by Brandon Gregg's script (Check Readme). The input is sampling data
    The format should be the following (with or without newline,space and indention): 
    
        //Application and thread id
        command-pid/tid;
    
            //bottom of stack
            func1 [library] or [library] 
                timestamp start
                timestamp end; 
            func2 [library] or [library] 
                timestamp start
                timestamp end;
            ...;
            funcn [library] or [library] 
                timestamp start
                timestamp end
            //Top of stack
    
            sample count

Output: A tracing centric tree structure 
    The format should be the following (with or without newline,space and indention): 
    
    {tid: 
        {root function name: 
            Function Tree
        }
    }

Function Tree: a instance of the Function Tree Class
root function name: the function and library associated with the root node of the function tree it keys to

Function Tree is a generic tree data structure where a node represents a single function call on a call stack

Each function tree node consists these basic pieces of information:
    - Function name
    - Library Name
    - Total Samples recorded for this function (Won't be used by converter)
    - Timestamps (An estimation. We can't get accurate timestamp from perf sampling)
    - List of child nodes
      - These child nodes represent the next function that was called in the call stack
      - There may be an arbitrary amount of child nodes because we merge call stacks with the same call sequence and 
        only diverge them once we detect a function that is different from known child functions
      - An example of this is seen below above the Function Tree definition

I use the root function name to key to a specific function tree because one may know the first function that was called 
for a call stack and thus they can find any call stack that may have resulted from that first function call.  

If I implemented just a list of function trees, the worst case search would be O(n) when trying to find the root 
function given a function name and library With the dictionary, the worst case search is O(1) since keying a dictionary 
uses a hash map
'''
# This will be set to true if the user acknowledges the use of timing information
# Otherwise, this remains false
# When true, I will check for data validation on timestamps to ensure that the user has valid timing data and then
# the timestamps will be set for each function node
# If false, then I will forego the timing information and completely remove it from the final data structure
# To save on data
useTimestamps = False


# REVIEW_TODO: I don't understand why it will be lost
# TODO Consider samples with call stacks that are a "substring" of another call stack
# EXAMPLE: [memcached];__libc_start_main [libc-2.31.so];[memcached];event_base_loop [libevent-2.1.so.7.0.0];[libevent-2.1.so.7.0.0] < - - will be lost in the final branch as it will be merged with the next line
#          [memcached];__libc_start_main [libc-2.31.so];[memcached];event_base_loop [libevent-2.1.so.7.0.0];[libevent-2.1.so.7.0.0];epoll_wait [libc-2.31.so];entry_SYSCALL_64_after_hwframe_[k];do_syscall_64_[k];__x64_sys_epoll_wait_[k];do_epoll_wait_[k];ktime_get_ts64_[k]
class FunctionTree:
    __slots__ = ['func', 'lib', 'timestamps', 'sampleTotal', 'childFuncs', 'execTime']

    def __init__(self, sampleCnt, lib, func='Unknown', timestamps=[], useTime=False):
        """
        Initialize a tree node
        :param sampleCnt: sample count
        :param lib:  library name
        :param func: function name
        :param timestamps: If useTime==true, this should not be empty
        :param useTime:
        """

        # self.root = True
        '''if func.startswith("_"):
            self.func = "!" + func
        else:
            self.func = func'''

        self.func = func
        self.lib = lib
        # If the user requests to use timing information, then I will expect that every function
        # has appropriate timestamp information, if not then we will exit and report the error
        if useTimestamps or useTime:
            if not timestamps:
                print("ERROR: No Timestamps available, exiting...")
                sys.exit()
            else:
                self.timestamps = []
                self.execTime = 0
                try:
                    for timeTup in timestamps:
                        # I do not know the units of the timestamps before hand, so may need to specify time units afterwards
                        # or settle on one time unit to use
                        self.addTimestamps(timeTup)
                except ValueError:
                    print("ERROR: Timestamps are not floats, exiting...")
                    sys.exit()
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
                        child.addTimestamps(timestamps)
                    return child
        # Haven't seen this child before
        newNode = FunctionTree(sampleCnt, lib, func, [timestamps])
        self.childFuncs.append(newNode)
        return newNode
        # cnt += 1

    def addTimestamps(self, timestamps):
        try:
            timestamps = (float(timestamps[0]), float(timestamps[1]))
        except ValueError:
            print("ERROR: Timestamps are not floats, exiting...")
            sys.exit()
        # print(timestamps)
        self.timestamps.append(timestamps)

    # Method for updating the timestamps of the node it was called by merging timestamps
    def updateTimestamps(self):
        # If data passes the previous test, then we know that the timestamps always occur after the start of execution
        # of the current child function
        # This makes sense since data tends to happen sequentially or in parallel, but there is always a
        # forward (time) moving causal relationship with the data, thus data appearing in the past makes no sense
        # unless data is out of order, but I do not know why one would want out of order data

        '''

        These are the various cases that have to be considered in regards to the timestamp merging
            ex: |---------|
               start     end
            |-------|   |-------|
            
            2 timestamps tuples, but they are not overlapping
            
            Current idea for solution: Store each timestamp tuple separately if they do not overlap
            AND the new timestamp tuple occurs after the current first stored timestamp tuple
            These tuples will be stored in a list in the self.timestamps attribute and it will be sorted by the
            start timestamp of each tuple in ascending order.
            Then whenever I need to update timestamps, I have to iterate through the timestamp list.

            # TODO: REVIEW: I have doubts about this

            # TODO: REVIEW: Some parts of the description is irrelevant with this function, which may cause confusion for readers
            The reason why I want to use a list is because I do not want to simply extend the first timestamp tuple
            with the second tuple (by swapping the end timestamp with the second end timestamp). If I do this, then
            this would result in including that break between the timestamp tuples (seen above). This break is not
            a part of the true duration of the timestamps and would bloat the true execution time of the function call.
            
            Whenever I do operations on a function's execution time, I want to be working in the realm of the original 
            execution time (from actual function start to function termination). 
            The reason why I want to do it this way is because this allows me to accurately redistribute
            execution times to obtain the true execution time of a function (The actual time spent in the function 
            and not in other functions) since child functions run during the original execution time (meaning it starts 
            after its parent) of its parent function. 
            It would not make sense to include those breaks because the child functions never run in those 
            breaks as I explained, they should run during the original execution times of the parent function
            
            or
            
            |-------|       or  |---------|--------|
                |--------|    
            
            2 timestamps tuples, but they are overlapping or they appear right after another
            
            Current idea for solution: Simply extend the first tuple with by swapping the end timestamp of the
            first tuple with the second end timestamp
            
            changing:
            |-------|
                |--------|
            to:
            |------------|
            
            If we have timestamps like this:
            
            |-----------|
               |-----|
               
            Then do nothing as the second timestamps is already covered by the first timestamps
            
                |---------|
            |-----|
            
            In this case, I declare these as impossible as my trees are stored on a thread-specific basis, so 
            function calls must occur sequentially. 
            Since these function calls are only associated with a specific thread.
        '''
        # Below only handles the case where the end and start match up for now

        '''
        General idea for merging these tuples:
        
        Assumptions:
           - List is sorted in ascending order by start timestamp
        
        As we find tuples that match this form:
        |--------|-------|
        
        we will save the earliest start and the latest end timestamp
        Then replace all of the tuples that were merged with the new merged tuple which contains the earliest start 
        timestamp and the latest end timestamp
        '''
        # TODO: REVIEW: This should be unnecessary, since timestamp is always incremental
        # TODO: REVIEW: Change this to checking whether timestamps are sorted. Because if not there should be an error.
        sorted(self.timestamps)
        # TODO: REVIEW: I think this should be renamed to prevStart and prevEnd
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

    # Special method for when print is called on a node, simply prints all of the attributes
    def __str__(self):
        # print("Noot")
        retStr = ""
        for att in dir(self):
            try:
                if not att.startswith('__') and not callable(getattr(self, att)):
                    retStr = f"{att}: {getattr(self, att)} " + retStr
            except AttributeError:
                continue
        return retStr

    # This is a private method to retrieve all of the attributes (aside from the child function list)
    # and store them into a dictionary which is used for serialization
    def __retrieveAttrs(self):
        if useTimestamps:
            return {"func": self.func, "lib": self.lib, "sampleTotal": self.sampleTotal, "timestamps": self.timestamps,
                    "execTime": self.execTime, "childFuncs": []}
        else:
            return {"func": self.func, "lib": self.lib, "sampleTotal": self.sampleTotal, "childFuncs": []}

    # This method will convert the current function tree into a serializable json format
    # This method assumes that serialize was called from the root node of the function tree
    # If called at any depth aside from root level, information will be lost about nodes prior the the node serialize
    # was called from
    def serialize(self):
        tempList = []
        if len(self.childFuncs) <= 0:
            return self.__retrieveAttrs()
        for childFunc in self.childFuncs:
            tempList.append(childFunc.serialize())
        aDict = self.__retrieveAttrs()
        aDict["childFuncs"] = tempList
        return aDict

    def __setExecTime(self):
        for timeTup in self.timestamps:
            self.execTime += timeTup[1] - timeTup[0]
        return

    @staticmethod
    def reconstructNodefromDict(j_Dict, useTime=False):
        if useTime:
            node = FunctionTree(sampleCnt=j_Dict["sampleTotal"], lib=j_Dict["lib"], func=j_Dict["func"],
                                timestamps=j_Dict["timestamps"], useTime=useTime)
            # node.__setExecTime()
            node.execTime = j_Dict["execTime"]
        else:
            node = FunctionTree(sampleCnt=j_Dict["sampleTotal"], lib=j_Dict["lib"], func=j_Dict["func"])

        if len(j_Dict["childFuncs"]) <= 0:
            return node
        for child in j_Dict["childFuncs"]:
            node.childFuncs.append(FunctionTree.reconstructNodefromDict(child, useTime))
        return node

    def __get_leaf_nodes(self, leaves):
        if not self.childFuncs:
            nodeDict = self.__retrieveAttrs()
            del nodeDict['childFuncs']
            leaves.append(nodeDict)
            return
        for child in self.childFuncs:
            child.__get_leaf_nodes(leaves)

    def retrieveLeaves(self):
        leaves = []
        self.__get_leaf_nodes(leaves)
        return leaves


# ============================================ Data Cleanup and Organization ===================================


def checkSuffix(func):
    """
    If the stack collapsing script was called with additional annotation for kernel, inlined and jitted functions,
    then I will declare the function's library as kernel, inlined, or jitted respectively and remove the suffix
    If a normal input of function [library] or [library] was passed in then, simply return 0 to indicate nothing was done

    # TODO: Review: I think it's better that checkSuffix only handles the first entry. And leave timestamp parsing to
    # parseFunc. (Just for clearity)

    @param func: function name
    @return: Several scenarios
    Function name without _[k], _[i] or _[j], return: 0
    Function name with _[k], _[i] or _[j], and with timestamp, return: [FunctionName, ModuleName (Either Kernel, Inlined or Jit), StartTime, EndTime]
    Function name with _[k], _[i] or _[j], but without timestamp, return: [FunctionName, ModuleName (Either Kernel, Inlined or Jit)]
    """
    suffix = {"_[k]": "Kernel", "_[i]": "Inlined", "_[j]": "Jit"}
    if any(suf in func for suf in suffix.keys()):
        aList = func.split()
        funcLen = len(aList[0])
        # print(func[0:-4], func, func[funcLen-4:], funcLen)
        # Module name without suffix
        moduleName = suffix.get(aList[0][funcLen - 4:], "error")
        if moduleName == "error":
            print("Error: Failed to find module name for kernel, inlined, or jitted function")
            sys.exit()  # Error occurred so print and exit
        if len(aList) > 1:
            # This means we have timestamp
            return [aList[0][:-4], moduleName, aList[1],
                    aList[2]]  # Remove the _[?] section from the function name, aList[1] and [2] contain timestamps
        else:
            return [aList[0][:-4], moduleName]
    return 0


def parseFunc(funcLine, index):
    """
    This will take a function string from the raw data and retrieve any information possible
    The possible information is: function name, library name, timestamps and sample count

    @param funcLine: Call stack list for a single line of perf output
    @param index:Which level of call stack in funcLine are we parsing
    @return:
    Differet senarios:
    funcLine[index] may have different senarios
    (4, Library Name): When the call stack contains only one symbol. For example [memcached]
    (2, Function Name,Module Name): For example __libc_start_main [libc-2.31.so]
    (3, Library Name,(StartTime,EndTime)): For example __libc_start_main 12345 123456
    (4, Function Name,Module Name,(StartTime, EndTime)): For example __libc_start_main [libc-2.31.so] 12345 123456
    """
    # Check for _[k], _[i], and _[j]
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

    # todo: REVIEW, The inner if should be taken out one level for clearity?
    elif tempLen == 4 or tempLen == 2:
        # If we have+ all possible inputs, function, library and timestamps
        if tempLen == 4:
            try:
                float(tempList[2])
                float(tempList[3])
            except ValueError:
                print("Error: Impossible input, exiting...")
                sys.exit()
            # if tempList[2] == "2130297.050642":
            #    print("End of file")
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


# =================================================================================================

# ======================================= Tree Construction Functions =============================

# Each line from the data represents a single branch in our function trees (due to the call stack),
# thus given a line from the data, we will construct this branch in our function tree
# This is done recursively by traversing down our function tree creating new nodes as needed until we reach
# the end of the call stack
# todo: REVIEW, for coding style, I think it's better to use a for loop outside rather than pass the entire list to
#  function and use recursion to increment the index.
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
    This is a helper function to create a new function sub-tree if one has not been created for the first function seen in the
    current line. If the first function has had a function sub-tree created for it then we will return that node, otherwise we
    have created a new node and will return that new node
    @param treeDict: key:tid, value: Tree
    @param sampleNum:
    @param funcLine:
    @return:
    """
    funcResult = parseFunc(funcLine, 0)
    timeEqual = False
    if funcResult[0] == 1 and useTimestamps:
        # funcResult = (1, func, lib, (time start, time end))
        # print(funcResult)
        try:
            time = (float(funcResult[3][0]), float(funcResult[3][1]))
        except ValueError:
            print("Error: Timestamps are not float, exiting...")
            sys.exit()
        rootNode = treeDict.setdefault(' '.join(funcResult[1:3]), FunctionTree(sampleCnt=0, lib=funcResult[2], func=funcResult[1],
                                                                               timestamps=[funcResult[3]]))

        if time not in rootNode.timestamps:
            rootNode.addTimestamps(time)

    elif funcResult[0] == 2 or (funcResult == 1 and not useTimestamps):
        # funcResult = (2, func, lib) or funcResult = (1, func, lib, (time start, time end)) while useTimestamps is false
        rootNode = treeDict.setdefault(' '.join(funcResult[1:]), FunctionTree(sampleCnt=0, lib=funcResult[2], func=funcResult[1]))
    elif funcResult[0] == 3 and useTimestamps:
        # funcResult = (3, lib, (time start, time end))
        try:
            time = (float(funcResult[2][0]), float(funcResult[2][1]))
        except ValueError:
            print("Error: Timestamps are not float, exiting...")
            sys.exit()
        # print(funcResult)
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(sampleCnt=0, lib=funcResult[1], timestamps=[funcResult[2]]))
        if time not in rootNode.timestamps:
            rootNode.addTimestamps(time)

    else:
        # funcResult = (4, lib) or funcResult = (3, lib, (time start, time end)) while useTimestamps is false
        rootNode = treeDict.setdefault(funcResult[1], FunctionTree(sampleCnt=0, lib=funcResult[1]))
    rootNode.sampleTotal += sampleNum
    return rootNode


def traverseTreeandUpdate(node):
    if not node.childFuncs:
        node.updateTimestamps()
        return

    for child in node.childFuncs:
        traverseTreeandUpdate(child)

    node.updateTimestamps()
    return


def mergeTimestamps(tidData):
    for treeDict in tidData.values():
        for tree in treeDict.values():
            traverseTreeandUpdate(tree)
    return


# =========================================================================================

# ======================================= Debugging Functions =============================

# Prints the branches of a node
def printBranch(nodeList, printrootchild, branchNum=0):
    if not nodeList:
        return
    for node in nodeList:
        if printrootchild:
            print("ROOT BRANCH: ")
        else:
            print(f"BRANCH #{branchNum}: ")
        print(node)
        printBranch(node.childFuncs, False, branchNum + 1)
    return


# Prints all of the function trees in a given dictionary of function trees
def printTree(treeList):
    num = 1
    for tree in treeList.values():
        print(f"TREE {num}:\n", tree)
        printBranch(tree.childFuncs, True)
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
    if fileName == ():  # TODO: Review: Maybe there are some version differences
        # If no file name then just default to opening a file in the repo
        # print(True)
        fileName = "/home/st/Projects/Scaler/libAnalyzer/tests/PerfTests/simplestExample.folded"
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
        global useTimestamps
        useTimestamps = True

    # currentTID = ''
    # print("Wow")
    with open(fileName) as fold:
        # a = 0
        lineNum = 1
        for line in fold:
            # print(lineNum)
            lineList = line.split(';')  # Line is semi-colon separated, thus we split using ;

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
            lastEleList = lineList[-1].split()  # The last element split by white space
            sampleNum = int(lastEleList[-1])  # Retrieve the line's sample number
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
            # else:
            #    print(lineNum)
            lineNum += 1
    if not tidData:
        print("ERROR: Tid Data Dict empty, exiting...")
        sys.exit()

    # for treeDict in tidData.values():
    #    printTree(treeDict)

    mergeTimestamps(tidData)

    # print("After Merging")
    # for treeDict in tidData.values():
    #    printTree(treeDict)
    with open(outFileName, 'w') as j_file:
        j_file.write(json.dumps(tidData, default=lambda tree: tree.serialize()))


if __name__ == "__main__":
    main()
