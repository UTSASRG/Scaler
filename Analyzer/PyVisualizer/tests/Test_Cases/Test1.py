import random
import string

def generateTimestamps():

    pass

def generateLibName(letter):
    return "lib" + letter

def generateFuncName(letter, libName):
    return "func" + letter + "_" + libName

def generateFakeData():
    outFile = open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/aggregator_Test1.folded", 'w')
    outLine = []
    alphabetU = list(string.ascii_uppercase)
    print(alphabetU)
    maxThreads = 20
    maxLines = 500
    minFuncs = 5
    startTime = 1
    fakeTIDList = random.sample(range(1,1000), random.randint(1,maxThreads))
    fakeTIDList.sort()
    num = 0
    for fakeTID in fakeTIDList:
        outLine.clear()
        outLine.append(str(fakeTID))
        for lineNum in range(random.randint(1,maxLines)):
            endTime = startTime + random.uniform(5.0, 10.0)
            # for letter in random.choices(alphabetU,k=random.randint(minFuncs, minFuncs + 3)):
            for letter in alphabetU[:minFuncs+1]:
                num += 1
                libLetter = random.choice(alphabetU[:minFuncs+1])
                lib = generateLibName(libLetter)
                funcName = generateFuncName(letter, lib)
                if num == 1:
                    timeList = [random.triangular(startTime, endTime, mode=startTime), random.triangular(startTime, endTime, mode=endTime)]
                    timeList.sort()
                    tempStartTime, tempEndTime = timeList[0], timeList[1]
                else:
                    timeList = [random.triangular(tempStartTime, tempEndTime, mode=tempStartTime), random.triangular(tempStartTime,tempEndTime, mode=tempEndTime)]
                    timeList.sort()
                    tempStartTime, tempEndTime = timeList[0], timeList[1]

                # "{} [{}] {} {:.3f} {:.3f}".format(funcName, lib, str(tempStartTime), str(tempEndTime))
                outLine.append('{} [{}] {:.6f} {:.6f}'.format(funcName, lib, tempStartTime, tempEndTime))
                # outLine.append(funcName + " [" + lib + "] " + str(tempStartTime) + " " + str(tempEndTime))
            num = 0
            startTime = endTime
            outFile.write(';'.join(outLine) + " 1\n")
            outLine.clear()
            outLine.append(str(fakeTID))

    outFile.close()

if __name__ == "__main__":
    generateFakeData()
