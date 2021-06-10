#By: John Diep
#Aggregates data into a json file
import collections
import json
import sys
import pprint
'''
Current Input Data
__________________
Perf Collapsed Stack
Input is in this style:
command-pid/tid;(bottom of stack) func1 [library] or [library]; func2 [library] or [library];...;(Top of stack) funcn [library] or [library] sample count

Must keep track of stack depth (?),
sample count per library (regardless of stack depth),
sample count per function with respect to library
Total samples recorded (easy)
thread ids (done)

Calculate Overhead as sample count / total samples
Final data structure:
{ TID: [ [^ {library: [ { func: sample cnt }, sample cnt ] } ]^, sample cnt ] }
{} = Dictionary
[^]^ = Stack
[] = list
'''

class threadData:
    def __init__(self):
        self.threadDict = {}
    def addTID(self, command):
        self.threadDict.setdefault(command, [[], 0])
    def updateLibrary(self, sampleCnt, depth, command, library, func="Unknown"):
        if func.startswith('_'):
            func = '!' + func
        # If depth is > length of call stack then add the library and its function as a new element to the stack at the top
        # Since this means that we do not have anything yet at this call stack depth
        callStack = self.threadDict[command][0]
        if depth > len(self.threadDict[command][0]):
            callStack.insert(0, {library: [{func: sampleCnt}, sampleCnt]})
        # If the stack already has an entry at the requested stack depth, then we need to make sure that the library
        # Does not already exists in the respective library dictionary, if not, create a new entry, if it does already exist
        # Check if the function does not yet exist, if it does, then update the sample cnt, if not
        # Then create a new entry for the function
        else:
            libDict = callStack[depth-1]
            if library in libDict.keys():
                funcDict = libDict[library][0]
                if func in funcDict:
                    funcDict[func] += sampleCnt
                else:
                    funcDict[func] = sampleCnt
                libDict[library][1] += sampleCnt
            else:
                libDict[library] = [{func: sampleCnt}, sampleCnt]
    def updateTIDSamp(self, TID, sampleCnt):
        self.threadDict[TID][1] += sampleCnt

#If the stack collapsing script was called with additional annotation for kernel, inlined and jitted functions,
#then I will declare the function's library as kernel, inlined, or jitted respectively and remove the suffix
#If a normal input of function [library] or [library] was passed in then, simply return 0 to indicate nothing was done
def checkSuffix(func):
    suffix = {"_[k]": "Kernel", "_[i]": "Inlined", "_[j]": "Jit"}
    if any(suf in func for suf in suffix.keys()):
        funcLen = len(func)
        # print(func[0:-4], func, func[funcLen-4:], funcLen)
        moduleName = suffix.get(func[funcLen-4:], "error")
        if moduleName == "error":
            print("Error: Failed to find module name for kernel, inlined, or jitted function")
            sys.exit() #Error occurred so print and exit
        return (func[:-4], moduleName)  # Remove the _[?] section from the function name
    return 0

def parseLine(rawfunc,TIDdata):
    # comm = rawfunc[0].split('-') #comm[0] = command name, comm[1] = pid/tid
    TIDdata.addTID(rawfunc[0])
    lastEle = rawfunc[-1].split()
    sampleNum = int(lastEle[-1])
    stackDepth = 0
    for func in rawfunc[1:]:
        stackDepth += 1
        # If the function is the last function, then replace tempList with the last element list from above,
        # otherwise just split func into a temporary list
        if func == rawfunc[-1]:
            suffRet = checkSuffix(''.join(lastEle[:-1])) #Remove the sample number from the func input since checkSuffix assumes the input is just the function and library
            tempList = lastEle[:-1]
        else:
            suffRet = checkSuffix(func)
            tempList = func.split()
        tempLen = len(tempList)
        if suffRet == 0:
            # run normally
            if tempLen > 2 or tempLen < 1:
                print("Error: Impossible input, exiting...")
                sys.exit()
            elif tempLen == 2:
                # tempList[0] = function name, tempList[1] = library name
                #If stack depth is larger than the current maximum depth of the current thread's call stack then make a new entry at stackDepth's depth
                TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], tempList[1], tempList[0])
            else:
                # tempList[0] = library name
                TIDdata.updateLibrary(command=rawfunc[0], library=tempList[0], sampleCnt=sampleNum, depth=stackDepth)
        else:
            # use kernel/inlined/jitted function we obtained from file
            TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], suffRet[1], suffRet[0])

    return sampleNum



def parseFold():
    tidData = threadData()
    fold = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/finalFold.folded")
    sampleTotal = 0
    currentTid = ''
    for line in fold:
        lineList = line.split(";")
        sampleCnt = parseLine(lineList, tidData)
        if sampleCnt == -1:
            print("Error: failed to parse functions")
            return
        if currentTid != lineList[0]:
            # Base case of when I initialized the currentTID var to '', no sample count has been accumulated yet, so I simply update currentTID
            if currentTid == '':
                currentTid = lineList[0]
            # Otherwise just run normally
            else:
                tidData.updateTIDSamp(currentTid, sampleTotal)
                currentTid = lineList[0]
                sampleTotal = 0
        sampleTotal += int(sampleCnt)
    tidData.updateTIDSamp(currentTid, sampleTotal)
    fold.close()
    # pprint.pprint(tidData.threadDict)
    with open("perfMemcachedData.json", 'w') as j_file:
        json.dump(tidData.threadDict, j_file)
    return
if __name__ == "__main__":
    parseFold()

