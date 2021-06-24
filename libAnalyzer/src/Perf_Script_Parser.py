# By: John Diep
'''
This is a helper script to convert perf script output into a input format for DataAggregator.py

'''
import sys
def libraryStrip(lib):
    #Can be in the form of ([library name]) or (library file path) or ([unknown])
    libraryName = lib.strip('()')
    libList = libraryName.split('/')
    return libList[-1]

def functionStrip(func):
    #This can be in the form of function name+offset or [unknown]
    funcList = func.split('+') # To the left of the + is the function name, to the right is the address offset
    return funcList[0]

def sortFinalList(final):
    return sorted(final, key= lambda x: x[0].split()[0].split('/')[1])


def writeToFile(final,finalFile):
    for line in sortFinalList(final):
        # print(line)
        finalFile.write(' '.join((line[0], str(line[1]) + "\n")))

#TODO Need to sort the final output by tid
def parseScript():
    perfOut = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/perf1.txt")
    finalFile = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/PerfTests/final_with_time.folded", 'w')
    commBool = True
    finalDict = {}
    outLine = []
    for line in perfOut:
        if line == "\n" or line == "\r\n":
            #finalFile.write(";".join(outLine) + " 1\n")
            aLine = ';'.join(outLine)
            if aLine in finalDict.keys():
                finalDict[aLine] += 1
            else:
                finalDict[aLine] = 1
            outLine = []
            commBool = True
            continue
        else:
            outList = line.split()
            if commBool:
                #Current line is line with commands and timestamps

                #Split in case the pid was reported with the tid, if it is just the pid, nothing happens, just a list with a single item is returned
                pidtidList = outList[1].split('/')

                #If pid was reported then simply join the first two elements (command and pid/tid) with a -
                if len(pidtidList) > 1:
                    commpidtid = "-".join(outList[:2])
                else:
                    #If pid was not reported then we will check if tid is in pidtidList, by trying to cast it to an int
                    #A value error is returned if the item is not a number indicating some error
                    try:
                        int(pidtidList[0])
                    except ValueError:
                        print("Error: Invalid input for pid and tid.")
                        sys.exit()
                    #The cast was successful, thus we know the tid was reported, then we just join the command and tid together with a -?/ which would end up being command-?/tid
                    commpidtid = '-?/'.join(outList[:2])
                # Save every item from the line except the event name, we will simply assume that the value before the event name is just cpu cycles
                outList = outList[2:-1]

                # Insert the command-pid/tid string to the start of the output list
                outList.insert(0, commpidtid)

                # Remove any : characters
                for ind in range(0,len(outList)):
                    outList[ind] = outList[ind].replace(':','')

                # Join all of the items into one string from outList and then append to the final list that will be written to a file
                finalComm = " ".join(outList)
                outLine.append(finalComm)
                # print("woop " + finalComm)
                # break
            else:
                # Line with functions
                # print(outList)

                # Clean up the function name and library names
                outList[-1] = libraryStrip(outList[-1])
                outList[1] = functionStrip(outList[1])

                # Deal with the case of when an unknown function was sampled or unknown library was sampled
                # If function is unknown
                if outList[1] == "[unknown]":
                    # If library is unknown then we will simply report unknown and in the data aggregator, it will be detected as an unknown library
                    if outList[-1] == "[unknown]":
                        outLine.insert(1,"[unknown]")
                    # If the library is known, then report the library by itself
                    else:
                        outLine.insert(1, outList[-1])
                # If function is known, run normally by joining the function name and library name together and reporting it
                # Generally if the function is known then the library is also known as being able to resolve the function name makes resolving the library name trivial
                else:
                    outLine.insert(1, " ".join(outList[1:]))
                # print(outList)
                # print(" ".join(outList[1:]))
        if commBool:
            commBool = False
    writeToFile(finalDict.items(), finalFile)
    perfOut.close()
    finalFile.close()

if __name__ == "__main__":
    parseScript()