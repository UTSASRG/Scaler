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

CURRENT PROBLEMS:
Timestamp Compatibility in data structure
- Currently I combine functions at the same stack depth from the same thread into one entry
  This means I retain the overall structure of the call stack but will be unable to retrieve a specific call stack
  I need the call stack of a specific sample in order to maintain causal order of the timestamps if needed
  
Timing Calculations
   - Overlapping Thread Problem
   - Time Duration
   - Total Execution Time

Information I have:
TID (IMPLEMENTED)
Lib Name (IMPLEMENTED)
Func ID/NAME (IMPLEMENTED)
Timestamps(Start/End) of functions (NOT YET IMPLEMENTED)
'''
# TODO produce way to identify a specific sample's call stack, also somehow implement timing information into data structure
class threadData:
    def __init__(self):
        self.threadDict = {}
    def addTID(self, command):
        self.threadDict.setdefault(command, [[], 0, 0])
    def updateLibrary(self, sampleCnt, depth, command, library, func="Unknown", callStackID=None, timeStart=None, timeEnd=None):
        if func.startswith('_'):
            func = '!' + func
        # If depth is > length of call stack then add the library and its function as a new element to the stack at the top
        # Since this means that we do not have anything yet at this call stack depth
        callStack = self.threadDict[command][0]
        duration = self.ExecDurationCalc(float(timeStart), float(timeEnd))
        if depth > len(self.threadDict[command][0]):
            #TODO Add in timestamps to func dictionary
            callStack.insert(0, {library: [{func: [sampleCnt, (timeStart, timeEnd, duration, callStackID)]}, sampleCnt, duration]})
        # If the stack already has an entry at the requested stack depth, then we need to make sure that the library
        # Does not already exists in the respective library dictionary, if not, create a new entry, if it does already exist
        # Check if the function does not yet exist, if it does, then update the sample cnt, if not
        # Then create a new entry for the function
        else:
            libDict = callStack[depth-1]
            if library in libDict.keys():
                funcDict = libDict[library][0]
                if func in funcDict:
                    funcDict[func][0] += sampleCnt
                else:
                    funcDict[func] = [sampleCnt, (timeStart, timeEnd, duration, callStackID)]
                libDict[library][1] += sampleCnt
                libDict[library][2] += duration
            else:
                libDict[library] = [{func: [sampleCnt, (timeStart, timeEnd, duration, callStackID)]}, sampleCnt, duration]
        try:
            float(duration)
        except ValueError:
            return None
        return duration

    def updateTID(self, TID, sampleCnt, duration=None):
        if duration == None:
            self.threadDict[TID][1] += sampleCnt
        else:
            self.threadDict[TID][1] += sampleCnt
            self.threadDict[TID][2] += duration
    def ExecDurationCalc(self, start, end):
        '''
            Calculates execution durations given a start and an end timestamp
            Simple calculation of end - start = duration

            What to consider:
            - Functions
            - Libraries
            - Threads
        '''
        return round(end - start, 6)

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
        return (aList[0][:-4], moduleName, aList[1], aList[2])  # Remove the _[?] section from the function name
    return 0

def parseLine(rawfunc,TIDdata,StackID):
    # comm = rawfunc[0].split('-') #comm[0] = command name, comm[1] = pid/tid
    TIDdata.addTID(rawfunc[0])
    lastEle = rawfunc[-1].split()
    sampleNum = int(lastEle[-1])
    duration = 0
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
            if tempLen > 4 or tempLen < 1:
                print("Error: Impossible input, exiting...")
                sys.exit()
            elif tempLen == 4:
                # tempList[0] = function name, tempList[1] = library name
                #If stack depth is larger than the current maximum depth of the current thread's call stack then make a new entry at stackDepth's depth
                dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], tempList[1], tempList[0], StackID, tempList[2], tempList[3])
            else:
                # tempList[0] = library name
                print(rawfunc)
                dura = TIDdata.updateLibrary(command=rawfunc[0], library=tempList[0], sampleCnt=sampleNum, depth=stackDepth,callStackID=StackID, timeStart=tempList[2], timeEnd=tempList[3])
        else:
            # use kernel/inlined/jitted function we obtained from file
            dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], suffRet[1], suffRet[0], StackID)
    if dura == None:
        return sampleNum
    else:
        return sampleNum, dura



def parseFold():
    tidData = threadData()
    # fold = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/finalFold.folded")
    fold = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/aggregator_Test1.folded")
    sampleTotal = 0
    currentTid = ''
    callStackID = 0
    duraTotal = 0
    for line in fold:
        lineList = line.split(";")
        outputParse = parseLine(lineList, tidData, callStackID)
        if len(outputParse) != 2:
            sampleCnt = outputParse
            duration = 0
        else:
            sampleCnt, duration = outputParse
        if sampleCnt == -1:
            print("Error: failed to parse functions")
            return
        if currentTid != lineList[0]:
            # Base case of when I initialized the currentTID var to '', no sample count has been accumulated yet, so I simply update currentTID
            if currentTid == '':
                currentTid = lineList[0]
            # Otherwise just run normally
            else:
                if duraTotal > 0:
                    tidData.updateTID(currentTid, sampleTotal, duraTotal)
                else:
                    tidData.updateTID(currentTid, sampleTotal)
                currentTid = lineList[0]
                sampleTotal = 0
            callStackID = 0
        callStackID += 1
        sampleTotal += int(sampleCnt)
        duraTotal += duration
    if duraTotal > 0:
        tidData.updateTID(currentTid, sampleTotal, duraTotal)
    else:
        tidData.updateTID(currentTid, sampleTotal)
    fold.close()
    # pprint.pprint(tidData.threadDict)
    # with open("perfMemcachedData.json", 'w') as j_file:
    with open("fakeData.json", 'w') as j_file:
        json.dump(tidData.threadDict, j_file)
    return
if __name__ == "__main__":
    parseFold()

