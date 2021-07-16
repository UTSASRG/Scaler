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
command-pid/tid;(bottom of stack) func1 [library] or [library] timestamp start timestamp end; func2 [library] or [library] timestamp start timestamp end;...;(Top of stack) funcn [library] or [library] timestamp start timestamp end sample count

Must keep track of stack depth (done),
sample count per library (regardless of stack depth), (done)
sample count per function with respect to library (done)
Total samples recorded (done)
thread ids (done)

Final data structure:
{ TID: [ [^ {library: [ { func: [(sampleCnt, callStackID, timestamps, duration)] }, sample cnt, duration ] } ]^, sample cnt, duration, max call stack ID] }
{} = Dictionary
[^]^ = Stack (Right end is the top of the stack)
[] = list

This final data structure maintains the call stack structure on a thread specific basis.
One can traverse the data structure by first searching for a thread and the value associated with it is a list which contains a stack, thread total sample count, thread execution time and max # of unique call stacks recorded + 1
The stack in this list maintains the call stack structure of a function call. The index of an element is associated with call stack depth via this formula: index + 1 = stack depth

Thus to find the function and/or library that was called at the top of the stack one can traverse the stack from the right to the left end (in reverse) as long as they know a sample's specific call stack ID

Within the stack, each index contains a dictionary with a key:value form of library:[]

The values of this dictionary is a list containing another dictionary with the key:value form of function:[]

The rest of the elements in this library value list are the library's total sample count, and total execution time

Then for the function value list, it will contain an arbitrary # of tuples which contain a sample count, call stack ID, timestamp tuple = (start,end) and total execution time

These sample counts, timestamps and durations are all associated with the call stack ID in the tuple, meaning that one can retrieve the information of a specific sample/trace

One Caveat: In the case of passing in output from Bradon Gregg's stack collapse-perf script, instead of grabbing a specific sample/trace, one will grab the information of a unique call stack which will NOT have timing information

CURRENT PROBLEMS:
Timestamp Compatibility in data structure
- *SOLVED* Currently I combine functions at the same stack depth from the same thread into one entry
  This means I retain the overall structure of the call stack but will be unable to retrieve a specific call stack
  I need the call stack of a specific sample in order to maintain causal order of the timestamps if needed 
  
Timing Calculations
   - Overlapping Thread Problem
   - *SOLVED* Time Duration
   - *SOLVED* Total Execution Time

Information I have:
TID (IMPLEMENTED)
Lib Name (IMPLEMENTED)
Func ID/NAME (IMPLEMENTED)
Timestamps(Start/End) of functions (IMPLEMENTED)
'''

# Functions for getting function information, library or thread information

#This obtains the sample counts for all functions associated with a specific library and thread
def getFuncSampleCnts(aDict, TID, Lib):
    funcDict = {}
    totalFuncSamps = 0
    for libs in aDict[TID][0]:
        for library, val in libs.items():
            if library == Lib:
                for func, funcVals in val[0].items():
                    for timeTups in funcVals:
                        totalFuncSamps += timeTups[0]
                    if func in funcDict.keys():
                            funcDict[func] += totalFuncSamps
                    else:
                        funcDict[func] = totalFuncSamps
                    totalFuncSamps = 0
    return funcDict

# Does the same as the above function except aggregate durations
def getFuncDuras(aDict, TID, Lib):
    funcDict = {}
    totalFuncDuras = 0
    for libs in aDict[TID][0]:
        for library, val in libs.items():
            if library == Lib:
                for func, funcVals in val[0].items():
                    for timeTups in funcVals:
                        totalFuncDuras += timeTups[3]
                    if func in funcDict.keys():
                            funcDict[func] += totalFuncDuras
                    else:
                        funcDict[func] = totalFuncDuras
                    totalFuncDuras = 0
    return funcDict

# Does the same as the function version but it does it for libraries of a specific thread
def getLibSampleCnts(aDict, TID):
    libDict = {}
    for libs in aDict[TID][0]:
        # print(libs)
        for library, val in libs.items():
            if library in libDict.keys():
                libDict[library] += val[1]
            else:
                libDict[library] = val[1]
    return libDict

# This will search in specific for the last function call of a particular call stack and then aggregate the sample count for that function call
# If it is not found then it returns false otherwise it was found and it returns true
def searchCallstack(callstackID, libraries, libDict):
    # print("callstack id:", callstackID, "libraries: ", libraries)
    for library, val in libraries.items():
        for funcVals in val[0].values():
            for timeTups in funcVals:
                if timeTups[1] == callstackID:
                    if library in libDict.keys():
                        libDict[library] += timeTups[0]
                        return True
                    else:
                        libDict[library] = timeTups[0]
                        return True
    return False

#Aggregates the sample counts of the last library call of unique call stacks seen for a thread
def getLastLibSampleCnts(aDict, TID):
    libDict = {}
    callstackID = 0
    # The final call stack ID indicates the maximum number unique call stacks + 1 seen for a specific thread
    finalcallstackID = aDict[TID][3]
    # Search through the thread's call stack for unique call stacks until we have gone through all of the unique call stacks
    while (callstackID <= finalcallstackID):
        for libs in aDict[TID][0][::-1]:
            if searchCallstack(callstackID,libs, libDict):
                callstackID += 1
                break
    return libDict

#Same as the library sample count aggregator except for duration/execution times
def getLibDuras(aDict, TID):
    libDict = {}
    for libs in aDict[TID][0]:
        # print(libs)
        for library, val in libs.items():
            if library in libDict.keys():
                libDict[library] += val[2]
            else:
                libDict[library] = val[2]
    return libDict

# Same as the other sample count aggregators except for threads
def getTIDSampleCnts(aDict):
    aList = []
    for val in aDict.values():
        #aList.insert(0, val[1])
        aList.append(val[1])
    return aList

#Same as the other duration aggregators except for threads
def getTIDDuras(aDict):
    aList = []
    for val in aDict.values():
        #aList.insert(0, val[1])
        aList.append(val[2])
    return aList


class threadData:
    def __init__(self):
        self.threadDict = {}
    def addTID(self, command):
        self.threadDict.setdefault(command, [[], 0, 0, 0])
    def updateLibrary(self, sampleCnt, depth, command, library,  callStackID, func="Unknown", timestamps=()):
        if func.startswith('_'):
            func = '!' + func
        # If depth is > length of call stack then add the library and its function as a new element to the stack at the top
        # Since this means that we do not have anything yet at this call stack depth
        callStack = self.threadDict[command][0]
        duration = self.ExecDurationCalc(timestamps)
        if depth > len(self.threadDict[command][0]):
            callStack.append({library: [{func: [(sampleCnt, callStackID, timestamps, duration)]}, sampleCnt, duration]})
        # If the stack already has an entry at the requested stack depth, then we need to make sure that the library
        # Does not already exists in the respective library dictionary, if not, create a new entry, if it does already exist
        # Check if the function does not yet exist, if it does, then update the sample cnt and duration if we have it, if not
        # Then create a new entry for the function
        else:
            libDict = callStack[depth-1]
            if library in libDict.keys():
                funcDict = libDict[library][0]
                if func in funcDict:
                    funcDict[func].append((sampleCnt, callStackID, timestamps, duration))
                else:
                    funcDict[func] = [(sampleCnt, callStackID, timestamps, duration)]
                libDict[library][1] += sampleCnt
                if duration != None:
                    libDict[library][2] += duration
            else:
                libDict[library] = [{func: [(sampleCnt, callStackID, timestamps, duration)]}, sampleCnt, duration]
        # Test if duration was calculated, if not then return nothing, else return the duration value
        try:
            float(duration)
        except TypeError:
            return None
        return duration

    #Updates the thread entry of the max call stack id, sample counts and durations if we have it
    def updateTID(self, TID, sampleCnt, callstackID, duration=None):
        callstackID -= 1
        if duration == None:
            self.threadDict[TID][1] += sampleCnt
            self.threadDict[TID][3] += callstackID
        else:
            self.threadDict[TID][1] += sampleCnt
            self.threadDict[TID][2] += duration
            self.threadDict[TID][3] += callstackID
    #Calculates the duration/execution time given a pair of start and end timestamps
    def ExecDurationCalc(self, timestamps):
        '''
            Calculates execution durations given a start and an end timestamp
            Simple calculation of end - start = duration

            What to consider:
            - Functions
            - Libraries
            - Threads
        '''
        # Test if values for the timestamps were passed in, if not then return nothing
        try:
            start = timestamps[0]
            end = timestamps[1]
            # Test if the values in the timestamp tuple are actual floats, if not then return nothing
            try:
                start = float(start)
                end = float(end)
            except TypeError:
                return None
        except IndexError:
            return None
        # If all tests passed then calculate the difference between the two timestamps to 6 sig figs
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
        if len(aList) > 1:
            return (aList[0][:-4], moduleName, aList[1], aList[2])  # Remove the _[?] section from the function name, aList[1] and [2] contain timestamps
        else:
            return (aList[0][:-4], moduleName)
    return 0

def parseLine(rawfunc,TIDdata,StackID,currentTID):
    # This is for the edge case where we read new threads in the traces/samples, but have not yet reset the call stack IDs
    # Thus if we detect a change then we reset the call stack IDs
    if currentTID != rawfunc[0]:
        if currentTID != '':
            StackID = 0

    # Preliminary setup, grabs tid and command, grab the last element which contains the sample counts and initialize a stack depth variable for maintaining call stack structure
    TIDdata.addTID(rawfunc[0])
    lastEle = rawfunc[-1].split()
    sampleNum = int(lastEle[-1])
    stackDepth = 0

    #Iterate through every function in the line except for the first element as that's just the command and tid
    for func in rawfunc[1:]:
        # With each read function we move down the call stack thus we increment the stack depth
        stackDepth += 1
        # If the function is the last function, then replace tempList with the last element list from above,
        # otherwise just split func into a temporary list
        if func == rawfunc[-1]:
            suffRet = checkSuffix(''.join(lastEle[:-1])) #Remove the sample number from the func input since checkSuffix assumes the input is just the function and library
            tempList = lastEle[:-1]
        else:
            suffRet = checkSuffix(func)
            tempList = func.split()
        # For checking valid inputs
        tempLen = len(tempList)
        if suffRet == 0:
            # run normally
            if tempLen > 4 or tempLen < 1:
                print("Error: Impossible input, exiting...")
                sys.exit()
            elif tempLen == 4 or tempLen == 2:
                # tempList[0] = function name, tempList[1] = library name
                # or
                # tempList[0] = function name, tempList[1] = library name, tempList[2] = timestamp start, tempList[3] = timestamp end
                #If stack depth is larger than the current maximum depth of the current thread's call stack then make a new entry at stackDepth's depth
                if tempLen == 4:
                    try:
                        float(tempList[2])
                        float(tempList[3])
                    except ValueError:
                        print("Error: Impossible input, exiting...")
                        sys.exit()
                    dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], tempList[1], StackID, tempList[0], (tempList[2], tempList[3]))
                else:
                    dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], tempList[1], StackID, tempList[0])
            else:
                # tempList[0] = library name
                # or
                # tempList[0] = library name, tempList[1] = timestamp start, tempList[2] = timestamp end
                # print(rawfunc)
                if tempLen == 3:
                    try:
                        float(tempList[1])
                        float(tempList[2])
                    except ValueError:
                        print("Error: Impossible Input, exiting...")
                        sys.exit()
                    dura = TIDdata.updateLibrary(command=rawfunc[0], library=tempList[0], sampleCnt=sampleNum, depth=stackDepth,callStackID=StackID, timestamps=(tempList[1], tempList[2]))
                else:
                    dura = TIDdata.updateLibrary(command=rawfunc[0], library=tempList[0], sampleCnt=sampleNum,
                                                 depth=stackDepth, callStackID=StackID)
        else:
            # use kernel/inlined/jitted function we obtained from file
            if len(suffRet) == 4:
                dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], suffRet[1], StackID, suffRet[0], (suffRet[2], suffRet[3]))
            else:
                dura = TIDdata.updateLibrary(sampleNum, stackDepth, rawfunc[0], suffRet[1], StackID, suffRet[0])

    if dura == None:
        return sampleNum, None
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
    # num = 0
    for line in fold:
        '''
        if num > 2:
            break
        '''
        lineList = line.split(";")
        outputParse = parseLine(lineList, tidData, callStackID, currentTid)
        # pprint.pprint(tidData.threadDict)
        if outputParse[1] == None:
            sampleCnt = outputParse[0]
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
                    tidData.updateTID(currentTid, sampleTotal, callStackID, duraTotal)
                else:
                    tidData.updateTID(currentTid, sampleTotal, callStackID)

                # change the current tid to the one we just swtiched to
                currentTid = lineList[0]
                # reset the total samples and call stack ID and total duration calculated for a thread
                # When we read a new thread, we initially passed the previous call stack id variable and reset then in this scope,
                # we use the previous call stack id to update the previous thread then we reset
                sampleTotal = 0
                callStackID = 0
                duraTotal = 0

        # Increment the callstack id, total samples read for a thread and total execution time for a thread
        # When we read a new thread, we already set the first line read for the new thread with a callstackID of 0,
        # thus the next line will receive an ID of 1
        callStackID += 1
        sampleTotal += int(sampleCnt)
        duraTotal += duration
        # num += 1
    # print(callStackID)

    # The last thread has not yet been updated, thus we update it
    if duraTotal > 0:
        tidData.updateTID(currentTid, sampleTotal, callStackID, duraTotal)
    else:
        tidData.updateTID(currentTid, sampleTotal, callStackID)
    # Close files and write our results to a json file
    fold.close()
    # pprint.pprint(tidData.threadDict)
    # with open("perfMemcachedData.json", 'w') as j_file:
    with open("../fakeData.json", 'w') as j_file:
       json.dump(tidData.threadDict, j_file)
    return
if __name__ == "__main__":
    parseFold()

