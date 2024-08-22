import os
import re

dataDir = '/home/steven/Projects/scalerbench/benchmarklog/ScalerPerfArtifectforDedupAndFerretAfterThreadGroupAdjustment 2023-11-27_16-19-42-594494-UTC-0500'
sampleCountRegex = re.compile('\(\d+.samples\)')

resultDict = {}
for applicationName in os.listdir(os.path.join(dataDir, 'Perf')):
    resultDict[applicationName] = list()
    for testTime in os.listdir(os.path.join(dataDir, 'Perf', applicationName)):
        with open(os.path.join(dataDir, 'Perf', applicationName, testTime, 'stderr.txt'), 'r') as f:
            lastLine = f.readlines()[-1]
            regexFoundLocation = sampleCountRegex.findall(lastLine)
            assert (len(regexFoundLocation) == 1)
            resultString = regexFoundLocation[0]
            resultString = resultString[1:-1]
            resultDict[applicationName].append(int(resultString.split()[0]))

for applicationName, countList in resultDict.items():
    print(applicationName, sum(countList) / len(countList),sep='\t')
