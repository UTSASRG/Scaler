# By: John Diep
'''
This is a helper script to convert perf script output into a input format for DataAggregator.py
Sorts lines by TID in ascending order
'''
import sys
import tkinter as tk
from tkinter import filedialog
import pprint

useTimestamps = False

def libraryStrip(lib):
    """
    Strips perf library entries of the parentheses and then removes the file path and
    simply returns the file name.
    :param lib: a library entry from a perf script output
    :return: A library file name
    """
    #Can be in the form of ([library name]) or (library file path) or ([unknown])
    libraryName = lib.strip('()')
    libList = libraryName.split('/')
    return libList[-1]

def functionStrip(func):
    """
    Strips a perf function name entry
    Simply strips the address offset that perf script adds to the function name.
    :param func:
    :return: The function name without the offset value
    """
    #This can be in the form of function name+offset or [unknown]
    funcList = func.split('+') # To the left of the + is the function name, to the right is the address offset
    return funcList[0]

def writeToFile(final, finalFile):
    """
    Writes the final dict to finalFile
    :param final: Final output dictionary
    :param finalFile: output file object from open()
    :return: nothing
    """
    # pprint.pprint(final)
    # pprint.pprint(final.keys())
    for key, lineDict in final:
        # print(key, lineDict)
        for line, sampleNum in lineDict.items():
        # print(line)
            finalFile.write(' '.join((';'.join([key, line]), str(sampleNum) + "\n")))

def addTimestamps(oldLineDict, commpidtid, finalDict, timestampDict):
    """
    Will tack on a timestamp tuple to each function name in the function line stored at oldLineDict[commpidtid]

    The rule is this:
    For a given sample, we will take the sample's timestamp as the start timestamp
    and the end timestamp will be the next sample that shares the same command and tid.

    :param oldLineDict: A dictionary with a key-value pair of commpidtid:last call stack that does not have timestamps
    :param commpidtid: A string of command-pid/tid
    :param finalDict: The final output dictionary
    :param timestampDict: A dictionary with a key-value pair of commpidtid:timestamp tuple to be attributed
    :return: nothing
    """
    '''if oldLine == "":
        print("ERROR: No Line detected, exiting...")
        sys.exit()'''
    # keyList = oldLine.split(';')

    # Get a list of all of the function names and library names
    keyList = oldLineDict[commpidtid].split(';')

    # index = 1
    # Insert our timestamp tuple into the previously read function line for commpidtid
    for index, func in enumerate(keyList):
        # keyList[index] = f"{func} {timestampTuple[0]} {timestampTuple[1]}"
        keyList[index] = f"{func} {timestampDict[commpidtid][0]} {timestampDict[commpidtid][1]}"
        # index += 1
    # Replace the old entry in the final dict with the new function line that now has timestamps
    finalDict[commpidtid][";".join(keyList)] = finalDict[commpidtid].pop(oldLineDict[commpidtid])
    return

def parseScript():
    """
    Parses perf script output.
    Collapses a sample's call stack into a one-liner with the form of:
    command-pid/tid;(bottom of stack) func1 [library] or [library] timestamp start timestamp end; func2 [library] or [library] timestamp start timestamp end;...;(Top of stack) funcn [library] or [library] timestamp start timestamp end sample count
    And writes it to an output file
    :return: Nothing
    """
    global useTimestamps

    root = tk.Tk()
    root.withdraw()
    fileName = filedialog.askopenfilename()
    if fileName == "":
        fileName = "C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/perf1.txt"
        outFileName = "C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/final_with_time.folded"
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

    timestampInput = input("Use Timestamps? y/n Default is n: ")
    if timestampInput == "y":
        useTimestamps = True

    perfOut = open(fileName)
    finalFile = open(outFileName, 'w')
    commBool = True
    finalDict = {}
    outLine = []
    timestampDict = {}
    oldLineDict = {}
    # oldLine = ""
    # Parse each line of perf script output
    for line in perfOut:
        # When we are about to read a new sample, there will be an empty new line read first,
        # When we detect this new line, we prepare for the new sample
        if line == "\n" or line == "\r\n":
            # Join the processed lines from the sample by semi colon
            # The final dictionary will have a format like this: {"comm-pid/tid":{funcLine:sample count}}
            commpidtidold = outLine[0]
            funcLine = ";".join(outLine[1:])

            # This old line dict will contain the previously read line for each commpidtid entry
            oldLineDict[commpidtidold] = funcLine

            # If the line is unique then we set the sample count as 1, otherwise if we see the line repeated,
            # we will increment its current sample count by 1

            # Create an entry if it does not exist, if it does update it.
            """
            NOTE:
            In Python 3.7+, Dictionaries are Insertion Ordered. 
            Meaning that the first line we wrote into our output file will be the first sample that was read.
            This is guaranteed only in Python 3.7+ since I am using the built-in dictionary data structure.
            
            In older versions of Python, it may not be guaranteed and reliable. May need the OrderedDict data structure
            from the collections module to resolve that issue. This is only really necessary if one needs to maintain
            sample ordering.
            """
            if commpidtidold in finalDict.keys():
                if funcLine in finalDict[commpidtidold]:
                    finalDict[commpidtidold][funcLine] += 1
                else:
                    finalDict[commpidtidold][funcLine] = 1
            else:
                finalDict[commpidtidold] = {}
                finalDict[commpidtidold][funcLine] = 1
            # Reset the output line list for the next sample
            outLine = []
            # Reset this boolean because we are about to read a line with the command in it
            commBool = True
            continue
        else:
            outList = line.split()
            if commBool:
                # Current line is line with commands and timestamps

                # Split in case the pid was reported with the tid, if it is just the pid, nothing happens, just a list with a single item is returned
                pidtidList = outList[1].split('/')

                # If pid was reported then simply join the first two elements (command and pid/tid) with a -
                if len(pidtidList) > 1:
                    commpidtid = "-".join(outList[:2])
                else:
                    # If pid was not reported then we will check if tid is in pidtidList, by trying to cast it to an int
                    # A value error is returned if the item is not a number indicating some error
                    try:
                        int(pidtidList[0])
                    except ValueError:
                        print("Error: Invalid input for pid and tid.")
                        sys.exit()
                    # The cast was successful, thus we know the tid was reported, then we just join the command and tid together with a -?/ which would end up being command-?/tid
                    commpidtid = '-?/'.join(outList[:2])

                # Save every item from the line except the event name and event counter
                outList = outList[2:-2]
                # Insert the command-pid/tid string to the start of the output list
                outList.insert(0, commpidtid)

                # For simulating timing with perf script outputs
                # Remove any : characters
                for ind in range(0,len(outList)):
                    outList[ind] = outList[ind].replace(':', '')

                if useTimestamps:
                    # Add timestamps to the previous call stack for commpidtid
                    if commpidtid not in timestampDict:
                        timestampDict[commpidtid] = (float(outList.pop(1)),)
                    elif len(timestampDict[commpidtid]) < 2:
                        timestampDict[commpidtid] = (timestampDict[commpidtid][0], float(outList.pop(1)))
                        addTimestamps(oldLineDict, commpidtid, finalDict, timestampDict)
                    else:
                        timestampDict[commpidtid] = (timestampDict[commpidtid][1], float(outList.pop(1)))
                        addTimestamps(oldLineDict, commpidtid, finalDict, timestampDict)
                else:
                    outList.pop(1)  # Remove the timestamp

                # Join all of the items into one string from outList and then append to the final list that will be written to a file
                finalComm = " ".join(outList)
                outLine.append(finalComm)
            else:
                # Line with functions
                # print(outList)

                # Clean up the function name and library names
                outList[-1] = libraryStrip(outList[-1])
                # outList[1:-1] = '_'.join(outList[1:-1])

                # There is a rare case where the line with functions has a function that is not joined by an _
                # An example of this is where operator new or operator new[] is called.
                # There fore I will join all function name elements by '_' if this happens
                # Any function names that do not fit this case will simply not change.
                temp = '_'.join(outList[1:-1])
                del outList[1:-1]
                outList.insert(1, temp)

                outList[1] = functionStrip(outList[1])

                # Deal with the case of when an unknown function was sampled or unknown library was sampled
                # If function is unknown
                if outList[1] == "[unknown]":
                    # If library is unknown then we will simply report unknown and in the data aggregator, it will be detected as an unknown library
                    if outList[-1] == "[unknown]":
                        outLine.insert(1, "[unknown]")
                    # If the library is known, then report the library by itself
                    else:
                        outLine.insert(1, outList[-1])
                # If function is known, run normally by joining the function name and library name together and reporting it
                # Generally if the function is known then the library is also known as being able to resolve the function name makes resolving the library name trivial
                else:
                    if outList[1].startswith("_"):
                        outList[1] = "!" + outList[1]
                    outLine.insert(1, " ".join(outList[1:]))
                # print(outList)
                # print(" ".join(outList[1:]))
        if commBool:
            commBool = False
    if useTimestamps:
        # Throw away last line due to lack of timestamps
        for key in oldLineDict:
            del finalDict[key][oldLineDict[key]]
    print(len(finalDict), finalDict.keys())
    writeToFile(finalDict.items(), finalFile)
    perfOut.close()
    finalFile.close()

if __name__ == "__main__":
    parseScript()