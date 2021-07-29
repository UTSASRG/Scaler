"""
For parsing output from Scaler
"""
import DataAggregator_V2 as da
import tkinter as tk
from tkinter import filedialog
import struct
import os
import json
import pprint
import warnings

class InvocationTree:
    __slots__ = ["type", "libID", "funcAddr", "timestamps", "firstChildIndex", "totalChildren", "extraField1", "extraField2", "BadNode", "childFuncs"]
    def __init__(self, nodeDict):
        """
        Initializes an Invocation Tree Node.
        Will set the corresponding parameters using the nodeDict passed in.
        Will also create an empty child function list.
        :param nodeDict: A dictionary containing all of a node's data.
        """
        self.type = nodeDict["type"]
        self.libID = nodeDict["libID"]
        self.funcAddr = nodeDict["funcAddr"]
        self.timestamps = nodeDict["timestamps"]
        self.firstChildIndex = nodeDict["firstChildIndex"]
        self.totalChildren = nodeDict["totalChildren"]
        self.BadNode = nodeDict["BadNode"]
        if self.type == 1:
            # assert nodeDict["extraField1"] is None or nodeDict["extraField2"] is None, \
            assert "extraField1" not in nodeDict or "extraField2" not in nodeDict, \
                   "Type: Normal function should not have extra fields"

        if self.type == 2:
            # assert nodeDict["extraField1"] is not None and nodeDict["extraField2"] is not None, \
            assert "extraField1" in nodeDict and "extraField2" in nodeDict, \
                   "Type: Pthread function should have extra fields"
            self.extraField1 = nodeDict["extraField1"]
            self.extraField2 = nodeDict["extraField2"]

        if self.type == 3:
            # assert nodeDict["extraField1"] is not None and nodeDict["extraField2"] is None, \
            assert "extraField1" in nodeDict and "extraField2" not in nodeDict, \
                   "Type: Semaphore Function should have only one extra field."
            self.extraField1 = nodeDict["extraField1"]

        self.childFuncs = []

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
    def retrieveBadNodes(self):
        """
        This function will retrieve any bad nodes in our invocation tree starting from the node that called
        this function. Typically, this should be called from the root node.

        This function will also remove bad nodes from their parent's child function list and update the parent's total
        children count.

        This removal essentially removes the entire subtree that has the bad node as the root node from the main
        invocation tree.
        :return: A list of all the bad nodes that we retrieved from the main invocation tree. Assuming that
                 this is called from the root node.
        """
        if self.BadNode:
            return [self]

        if not self.childFuncs:
            return []

        badNodeList = [] # This list will store any bad nodes
        newChildList = [] # This list will store all children that are not bad nodes.
        for child in self.childFuncs:
            if not child.BadNode:
                newChildList.append(child)
            badNodeList = badNodeList + child.retrieveBadNodes()

        # Remove bad nodes by updating our child function list with only good nodes.
        self.childFuncs = newChildList
        # Since we removed the bad nodes, the total children of the current node changed.
        self.totalChildren = len(newChildList)
        return badNodeList

    def __retrieveAttrs(self):
        """
        This is a private method to retrieve all of the attributes (aside from the child function list)
        and store them into a dictionary which is used for serialization
        :return: A dictionary with all of the attributes and their value except for child functions, which an empty list
        is returned instead
        """
        tempDict = {}
        outputDict = {}
        for att in dir(self):
            try:
                if not att.startswith('__') and not callable(getattr(self,att)):
                    # retStr = f"{att}: {getattr(self, att)} " + retStr
                    if att != "childFuncs":
                        tempDict[att] = getattr(self, att)
                    else:
                        outputDict[att] = []
            except AttributeError:
                continue
        outputDict["nodeDict"] = tempDict
        return outputDict

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
        for childFunc in self.childFuncs:
            tempList.append(childFunc.serialize())
        aDict = self.__retrieveAttrs()
        aDict["childFuncs"] = tempList
        return aDict

    @staticmethod
    def reconstructFromDict(j_dict):
        """
        Reconstructs an invocation tree from serialized data in a json file
        :param j_dict: A dictionary with a serialized invocation tree.
        :return: The root node of the invocation tree.
        """
        node = InvocationTree(j_dict["nodeDict"])
        if not j_dict["childFuncs"]:
            return node
        for child in j_dict["childFuncs"]:
            node.childFuncs.append(InvocationTree.reconstructFromDict(child))
        return node

def getFileName(filePath):
    """
    Retrieves the file name from a file path string.
    If anything other than a file path is passed in, nonsense will be returned.
    :param filePath: A string containing a file path
    :return: File name, if a proper file path was passed in.
    """
    return filePath.split("/")[-1]

def convertToFunctionTree(node, jsonDict, functionNode=None):
    """
    Converts an Invocation Tree into a Function Tree from DataAggregator_V2

    An invocation tree by itself is mostly the same as a Function Tree. The Root node is always empty, so we skip that.
    The direct children of the root node essentially can be understood as the root node of a function tree and thus
    be understood as the start of a call stack.

    When Invocation trees are generated, nodes of the same function and library at the same stack level, will not be
    merged together. However, in the Function Tree, they will be merged.

    Therefore, during the construction of the Function Tree, we will be merging Invocation Tree nodes together to form
    one Function Tree node as long as they share the same function and library names.

    Therefore each branch will be a unique call stack, any sub path on the branch could also represent a recorded
    call stack.
    :param node: An Invocation Tree Node. Typically should be the root node.
    :param jsonDict: A dictionary containing the symbol data for the thread that the invocation tree node was recorded from
    :param functionNode: A Function Tree Node. This is only meant to be used in the function itself for recursion.
    :return: If used properly, then a dictionary containing our converted function trees with the form of:
             {"func name lib name": Function Tree Root Node}
    """
    # Check if the current node is the root, only the root will have a first child index of 1.
    if node.firstChildIndex == 1:
        # print(node)
        # print(functionNode)
        assert functionNode is None, "This default variable is only meant to be used inside the function."
        functionTreeDict = {}
        # functionTreeList = []
        # Create each root node of the function trees since the direct children of the invocation tree root node
        # represents the start of a function tree
        for child in node.childFuncs:
            # Grab the library name and function name
            symbolDict = getSymbol(jsonDict, child.libID, child.funcAddr)
            libName = symbolDict["fileName"]
            funcName = symbolDict["funcName"]
            timestampTup = (child.timestamps[0]/(10**6), child.timestamps[1]/(10**6))

            '''
            Create our new function tree root node
            As long as there are no other nodes that share the same function and library.
            If that is the case, then we will merge the two nodes by incrementing it's sample total by 1 and 
            add the current invocation node's timestamps to the timestamp list of the function tree node that
            shares function name and library name.
            
            Then we will expand that function tree node with the current invocation tree node's children.
            '''
            treeKey = f"{funcName} {libName}"
            if treeKey in functionTreeDict:
                functionTreeDict[treeKey].sampleTotal += 1
                functionTreeDict[treeKey].addTimestamps(timestampTup)
            else:
                functionTreeDict[treeKey] = da.FunctionTree(sampleCnt=1, lib=libName, func=funcName, timestamps=[timestampTup], useTime=True)

            childFuncNode = functionTreeDict[treeKey]
            # Now construct the function tree from the invocation tree by creating the children of the new function tree
            # root nodes
            for nextChild in child.childFuncs:
                convertToFunctionTree(nextChild, jsonDict, childFuncNode)

            # functionTreeList.append(childFuncNode)
        return functionTreeDict
    # Otherwise, we have normal nodes of the invocation tree
    else:
        # Grab the library name and function name
        symbolDict = getSymbol(jsonDict, node.libID, node.funcAddr)

        # functionNode is the parent of the current node, thus we add the current node to the parent's child nodes
        funcNode = functionNode.addNode(sampleCnt=1, lib=symbolDict["fileName"], func=symbolDict["funcName"], timestamps=(node.timestamps[0]/(10**6), node.timestamps[1]/(10**6)), useTime=True)

        # If we reached a leaf node then we simply add the leaf node to its parent's child nodes and then return
        if not node.childFuncs:
            return

        # create the children for the parent node
        for child in node.childFuncs:
            convertToFunctionTree(child, jsonDict, funcNode)
    return


# ================================================ Binary File Read Functions ================================
def handleRead(file, bytesToRead):
    """
    This will perform file.read(bytesToRead) and the case where we reach EOF.

    When we reach EOF, 3 things can happen
    Case 1: Normal read, nothing bad occurs, we read exactly the number of bytes we expected.
    Case 2: Read less than what was expected
            Sub Case: Read nothing => tried to read EOF. An empty string is returned.

    For Case 1: Simply return our data, nothing bad happened.
    For Case 2: Raise an exception because this script is designed to handle Scaler's binary output and that means
                that all the data I read should all fit into Case 1 and then we reach EOF while trying to perform
                any of our non 1 byte reads, this means that data is somehow missing or corrupted.
                This is because scaler will dump all of its Invocation Tree Nodes as an array of data and thus at the
                EOF, we should have read the last element in full and the next read should only return an empty string.
    For Case 2's Sub Case: if this occurs on any of the Non 1-byte reads, this should indicate an error because we
                           should only be reading the EOF when trying to read a new node which is indicated
                           by a 1 byte read for Type.
                           Therefore when we detect a successful read of the file by the EOF empty string return
                           when trying to read in our type value.
    :param file:
    :param bytesToRead:
    :return:
    """
    data = file.read(bytesToRead)
    if bytesToRead != 1:
        if len(data) < bytesToRead:
            raise Exception("File Read: Data is missing or corrupted.")
    return data

def readNode(file, isRoot=False):
    """
    This function will create a dictionary of a node's data based on the type of node that is being read.
    We will also do an error check on the data to indicate whether or not the node we are reading in is a bad node.
    A bad node is a node that is missing information.
    Missing information is generally indicated by a read value being -1.

    Since the root node is supposed to be empty of information.
    I use a isRoot parameter to avoid declaring that the root node is a bad node.
    :param isRoot: Boolean. It tells us if the node we are about to read is the root node. This can only happen once.
                   Since the root node is the very first node we read from a thread binary file.
    :param file: A binary file generated by Scaler
    :return: A dictionary of a node's data + a BadNode boolean.
    """
    typeData = handleRead(file, 1)
    if not typeData:
        return None
    type = struct.unpack('b', typeData)[0]
    assert type > 0 or type <= 3, 'Unspecified Format or corrupted type value'

    outputDict = {}
    outputDict['type'] = type
    outputDict["libID"] = struct.unpack('q', handleRead(file, 8))[0]
    outputDict["funcAddr"] = struct.unpack('q', handleRead(file, 8))[0]
    if any(val < 0 for val in outputDict.values()) and not isRoot:
        outputDict["BadNode"] = True
    outputDict["timestamps"] = (struct.unpack('q', handleRead(file, 8))[0], struct.unpack('q', handleRead(file, 8))[0])
    if any(timestamp < 0 for timestamp in outputDict["timestamps"]) and not isRoot:
        outputDict["BadNode"] = True
    outputDict["firstChildIndex"] = struct.unpack('q', handleRead(file, 8))[0]
    outputDict["totalChildren"] = struct.unpack('q', handleRead(file, 8))[0]
    if type == 2:
        outputDict["extraField1"] = struct.unpack('q', handleRead(file, 8))[0]
        outputDict["extraField2"] = struct.unpack('q', handleRead(file, 8))[0]
    elif type == 3:
        outputDict["extraField1"] = struct.unpack('q', handleRead(file, 8))[0]
    if "BadNode" not in outputDict:
        assert outputDict["timestamps"][1] >= outputDict["timestamps"][0], "Data Error: End timestamp is < start timestamp."
        outputDict["BadNode"] = False
    else:
        warnings.warn("Bad Node Detected")
    return outputDict
# ============================================= Json Handling Functions ================================
def loadJsons(jsonFileList, outFilePath):
    """
    Loads the symbol.json file data in.
    Will perform error checks to see if there are additional json files that is not symbol.json
    To access the symbol.json data in the output dictionary. Use the key "symbol".
    :param jsonFileList: A list with the file paths to the symbol.json
    :return: A dictionary with the symbol.json data.
    """
    # assert len(jsonFileList) == 2, "There should only be 2 json files, a symbol.json and commonFuncId.json"
    outputDict = {}
    for jsonFile in jsonFileList:
        symbolCheck = getFileName(jsonFile).startswith("symbol")
        errorLogCheck = getFileName(jsonFile).startswith("errorLog")
        InvocationjsonCheck = getFileName(jsonFile).startswith("InvocationTree")

        assert symbolCheck ^ errorLogCheck ^ InvocationjsonCheck ^ (getFileName(outFilePath) == getFileName(jsonFile)),\
               "One or more files are not symbol.json or errorLog.json"
        if symbolCheck:
            with open(jsonFile) as j_file:
                    outputDict[getFileName(jsonFile)[:-5]] = json.load(j_file)
    return outputDict

def getSymbol(jsonDict, libID, funcAddr):
    """
    Handles data retrieval from symbol data.
    :param jsonDict: The dictionary with the symbol data
                     ex: {libID: {fileName:file path, funcNames:{funcID:function name}}}
    :param libID: An Invocation Tree's LibID (may also be referred to as FileID)
    :param funcAddr: An Invocation Tree's FuncID
    :return: The data retrieved based on libID and funcAddr as a dictionary
    """
    libID = f'{libID}'
    funcAddr = f'{funcAddr}'
    try:
        symbolDict = jsonDict[libID]
        try:
            output = {"fileName": getFileName(symbolDict["fileName"]), "funcName": symbolDict["funcNames"][funcAddr]}
        except TypeError:
            output = {"fileName": getFileName(symbolDict["fileName"]), "funcName": "Unknown"}
    except KeyError:
        raise Exception("Key(s) missing from dictionary. Potentially error with libID or funcAddr or data corruption or loss.")
    assert output["funcName"] is not None, "Function name was not resolved."
    assert output["fileName"] is not None, "File/Library name was not resolved."
    return output
# ================================================================================================
# ====================================== Debugging Functions =====================================

def printTreeByLevel(node):
    """
    Performs Level Order Traversal to print each level
    :param node: An Invocation Tree Node. Typically the root node should be passed in to print the whole tree.
    :return: Nothing
    """
    queue = []
    queue.insert(0, node)
    while queue:
        length = len(queue)
        while length > 0:
            aNode = queue.pop()
            print(aNode, end=" - ")
            for child in aNode.childFuncs:
                queue.insert(0, child)
            length -= 1
        print()
    return

def printTreeBranches(node):
    """
    Prints all unique branches in the Invocation Tree.
    :param node: An Invocation Tree Node. Typically the root node should be passed in.
    :return: Nothing
    """
    def getBranchesFromTree(node):
        """
        Retrieves all branches as a nested list with each element representing a unique branch of the tree,
        from root to leaf node.
        :param node: An Invocation Tree Node. Typically the root node should be passed in.
        :return: A nested list with each element representing a unique branch from root to node
                 (if the root node was initially passed in)
        """
        if not node.childFuncs:
            return [[node]]
        return [[node] + path for child in node.childFuncs for path in getBranchesFromTree(child)]

    branchList = getBranchesFromTree(node)
    for branch in branchList:
        for node in branch:
            print(f"LibID: {node.libID} BadNode: {node.BadNode}", end=" ")
        print()
    return

# =============================================================================================
def main():
    """
    Parse all of the binary data of a thread's invocation tree.
    The binary data is stored as a level-order traversal of the invocation tree.

    Thus we can simply read in the binary data like a level-order traversal and then reconstruct the invocation tree
    by building each level and queueing up each node's children as the level is built.

    Once the invocation tree is reconstructed, we will convert that tree into the DataAggregator's function tree
    data structure after we remove any invocation tree nodes that were missing information,
    which are dubbed as Bad Nodes.

    Each invocation tree may lead to the construction of multiple function trees.

    At the end of the script, the resulting invocation trees will be stored in a json file for use if needed.

    The bad nodes we removed is also stored into an error log json.

    Then the resulting function trees are written to an output file which was selected by the user.
    :return: Nothing
    """
    defaultCase = False

    nodeQueue = []

    tidInvocationDict = {} # Will contain a thread's invocation tree

    errorDict = {} # Will contain a thread's bad node sub-tree
    outputDict = {} # Will contain the final data structure to be loaded into a json file for converter.py

    root = tk.Tk()
    root.withdraw()
    print("Please select the directory with the corresponding json files and binary files")

    # Ask for the directory with all of binary files and json files in it
    directory = filedialog.askdirectory()
    if directory:
        # Get a list of all of the files in the directory
        # Then generate a list of all of the absolute file paths from the file list and directory
        fileList = os.listdir(directory)
        threadFileList = ["/".join([directory, file]) for file in fileList if file.lower().endswith(".bin")]

        stopBool = False
        while True:
            print("Please select an output json file.")
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

    else:
        # default case, for testing purposes
        threadFileList = ["C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/Test_Cases/Scaler_Tests/simpleProgram_0x7fa3b750de00.bin"]
        outFileName = "C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/Test_Cases/Scaler_Tests/simpleProgram.json"
        defaultCase = True

    if not defaultCase:
        jsonFiles = ["/".join([directory, file]) for file in fileList if file.lower().endswith(".json")]
        # print(jsonFiles)
        jsonDict = loadJsons(jsonFiles, outFileName)
        # pprint.pprint(jsonDict)

    for filePath in threadFileList:
        currentIndex = 0
        TID = getFileName(filePath).split('_')[-1][:-4]
        tidInvocationDict.setdefault(TID, None)
        outputDict.setdefault(TID, None)
        with open(filePath, 'rb') as scaler_input:
            """
            Below is essentially a level-order traversal, except we are constructing each level as we traverse to them.
            """
            # Read in root node first.
            nodeDict = readNode(scaler_input, isRoot=True)
            if nodeDict is not None:
                # print(nodeDict)
                nodeQueue.insert(0, InvocationTree(nodeDict))
                currentIndex += 1

            while len(nodeQueue) != 0:  # While the queue has items
                """
                Whenever we reach this outer while loop, we just queued up all of the children of the node
                we just popped off the queue.
                The inner loop's purpose is to load in the children of the all nodes of one level to the queue.
                Thus in essence, this is a level-order traversal, where we construct each layer as we load them in.
                """
                queueLen = len(nodeQueue)
                while queueLen > 0: # Handle the layer that has been loaded into the queue
                    node = nodeQueue.pop()
                    # The node we just popped off when the output dictionary has nothing in it, is the root node.
                    if tidInvocationDict[TID] is None:
                        tidInvocationDict[TID] = node

                    # Error check for if the first child index of our current node does not match our current index
                    # in the binary data. The current index value indicates our position in the node array that is
                    # represented in the binary data.

                    # print(currentIndex, node.firstChildIndex)
                    if node.firstChildIndex != -1:
                        assert currentIndex == node.firstChildIndex, \
                            "Current Node's First Child Index does not match current index. Potential Data Corruption or Loss."

                    # Load into the queue, all of the children of the current node from the binary data.
                    for i in range(node.totalChildren):
                        nodeDict = readNode(scaler_input)
                        assert nodeDict is not None, \
                            "Data Error: Reached EOF, when we expected more data. Potential Data Corruption or loss."
                        # Create the node we just read in from the binary file and inserted into the queue.
                        childNode = InvocationTree(nodeDict)
                        node.childFuncs.append(childNode)
                        nodeQueue.insert(0, childNode)

                        currentIndex += 1  # We just loaded in a new node, thus we increment our index
                    queueLen -= 1

        if not defaultCase:

            errorDict[TID] = tidInvocationDict[TID].retrieveBadNodes()
            # pprint.pprint(errorDict)

            funcTreeDict = convertToFunctionTree(tidInvocationDict[TID], jsonDict[f"symbol_{TID}"])

            assert outputDict[TID] is None, "Output dictionary should not have a value for tid yet."

            outputDict[TID] = funcTreeDict

    if not defaultCase:
        print("Writing to output files...")
        assert len(tidInvocationDict) > 0, "Dictionary of Invocation trees empty."
        with open("/".join([directory, "InvocationTree.json"]), 'w') as InvoTreeFile:
            InvoTreeFile.write(json.dumps(tidInvocationDict, default=lambda tree: tree.serialize()))

        if errorDict:
            with open("/".join([directory, "errorLog.json"]), 'w') as errorFile:
                json.dump(errorDict, errorFile)

        assert len(outputDict) > 0, "Output Dictionary is empty."

        # We completed parsing all of our invocation tree data and made our function trees.
        # Clean up the timestamps, if needed.
        da.mergeTimestamps(outputDict)

        with open(outFileName, 'w') as outFile:
            outFile.write(json.dumps(outputDict, default=lambda tree: tree.serialize(useTime=True)))
    else:
        with open(outFileName, "w") as test_file:
            test_file.write(json.dumps(tidInvocationDict, default=lambda tree: tree.serialize()))
    return

if __name__ == "__main__":
    main()
    '''root = tk.Tk()
    root.withdraw()
    directory = filedialog.askdirectory()
    fileList = os.listdir(directory)
    jsonFiles = ["/".join([directory, file]) for file in fileList if file.lower().endswith(".json")]
    jsonDict = loadJsons(jsonFiles)
    pprint.pprint(jsonDict)
    # print(getSymbol(jsonDict, 0, 0))'''
