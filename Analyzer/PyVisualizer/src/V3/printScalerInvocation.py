import os

from util.Parser.TimeOutputPrarser import readSymbolFiles, readTimingStruct

rootFolder = '/home/steven/Projects/scalerbench/benchmarklog/ScalerCounting 2023-11-23_14-01-56-509711-UTC-0500'

scalerDataFolder = None

appDataDirList = []

for root, dirs, files in os.walk(rootFolder):
    for dirName in dirs:
        if dirName.startswith('scalerdata_'):
            for v in root.split(os.sep):
                if v.startswith('Application'):
                    appDataDirList.append([v, os.path.join(root, dirName)])

for name, path in appDataDirList:
    recInfo = readSymbolFiles(path)
    count = 0
    for threadId in recInfo.threadIdList:
        curThreadRecArray, _ = readTimingStruct(path, threadId)
        count += sum([rec.count for rec in curThreadRecArray])
    print(name,'\t',count)