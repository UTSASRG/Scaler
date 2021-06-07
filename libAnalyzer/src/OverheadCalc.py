#By: John Diep
#Calculates overhead for each thread and

class ThreadData:
    def __init__(self):
        pass
def traverseDict(aDict):
    pass


def checkSuffix(func):
    suffix = {"_[k]": "kernel", "_[i]": "inlined", "_[j]": "jit"}
    if any(suf in func for suf in suffix.keys()):
        moduleName = suffix.get(func[-4:0], "error")
        if moduleName == "error":
            print("Error: Failed to find module name for kernel, inlined, or jitted function")
            sys.exit() #Error occurred so print and exit
        return func[:-4]  # Remove the _[?] section from the function name
    return 0

def parseline(rawfunc,threadDict):
    # comm = rawfunc[0].split('-') #comm[0] = command name, comm[1] = pid/tid
    #Add the command and tid as a new key to the function d
    if rawfunc[0] not in threadDict:
        threadDict[rawfunc[0]] = [{},0]
    lastEle = rawFunc[-1].split()
    sampleNum = int(lastEle[-1])
    for func in rawfunc[1:-1]:
        suffRet = checkSuffix(func)
        if suffRet == 0:
            # run normally
            tempList = func.split()
            tempLen = len(tempList)
            if tempLen > 2 or tempLen < 1:
                print("Error: Impossible input, exiting...")
                sys.exit()
            elif tempLen == 2:
                #tempList[0] = function name, tempList[1] = library name
                if tempList[1] not in threadDict[rawfunc[0]][0]:
                    threadDict[rawfunc[0]][0][tempList[1]] = [{},0]
                threadDict[rawfunc[0]][0][tempList[1]][0][tempList[]]

            else:
                #tempList[0] = function name or library name
        else:
            # use the kernel/inlined/jitted function we obtained from the line
    if len(lastEle) > 2:
        # run normally
    else:
        suffRet2 = checkSuffix(lastEle)
        if suffRet2 == 0:
            # run normally (Most likely an unknown function, so there is only the library name and the sample number)
        else:
            # use the kernel/inlined/jitted function we obtained from the line


def parseFold():
    threadSamples = {}
    funcSamples = {}

    fold = open("FlameMods2.folded")
    for line in fold:
        lineList = line.split(";")
        if parseLine(lineList, threadSamples) == -1:
            print("Error: failed to parse functions")
            return

    fold.close()

parseFold()

