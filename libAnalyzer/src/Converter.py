
from DataAggregator_V2 import FunctionTree, printTree
import pprint
import json
'''
This converter script will convert the final output data of Data Aggregator into an easy to use format
for charter, that way I pre-process all of the data so I can immediately create a pie chart.
Currently it will generate a dictionary of sample totals for libraries called for a specific thread
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

def main():
    with open("perfMemcachedData_V2.json", 'r') as j_file:
        tidData = json.load(j_file)

    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        global useTimestamps
        useTimestamps = True

    loadData(tidData)
    #pprint.pprint(tidData)
    # print(tidData)
    # for treeDict in tidData.values():
    #    printTree(treeDict)
    leafDict = getLeaves(tidData)
    #pprint.pprint(leafDict)
    libDict = getLibSampTot(leafDict)
    #pprint.pprint(libDict)
    libTotDict = {}
    for libraryDict in libDict.values():
        for lib, total in libraryDict.items():
            libTotDict.setdefault(lib, 0)
            libTotDict[lib] += total

    with open("chart_data.json", 'w') as chart_file:
        json.dump([libDict,libTotDict], chart_file, indent=4)


if __name__ == "__main__":
    main()