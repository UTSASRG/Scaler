import os
import pandas as pd
from struct import Struct
import struct
from typing import List
import numpy as np
import re
from TimingStruct import FileRecord, ExtSymRecord

pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')


# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/media/umass/datasystem/steven/Downloads/Perf-Parsec-Callgraph/freqmine/scalerdata'

df = pd.read_csv(os.path.join(scalerDataFolder, 'fileName.txt'))
fileNameList = df['pathName'].to_list()
pthreadFileId = parsePthreadId(fileNameList)

df = pd.read_csv(os.path.join(scalerDataFolder, 'symbolInfo.txt'))
symbolNameList = df['funcName'].to_list()
symbolFileIdList = df['fileId'].to_list()
symIdInFile = df['symIdInFile'].to_list()
realFileId = None
df = pd.read_csv(os.path.join(scalerDataFolder, 'symbolInfo.txt'))

threadIdList = []
for fileName in os.listdir(scalerDataFolder):
    if fileName.startswith('threadTiming_'):
        threadIdList.append(fileName[len('threadTiming_'): -4])

timingRecord = []  # Map file name to FileRecord struct


def generateTimingStruct(aggregatedTimeEntries):
    mainFileId = None
    global realFileId

    with open(os.path.join(scalerDataFolder, 'realFileId.bin'), 'rb') as f:
        byteArr1 = f.read()
        realFileId = list(struct.unpack_from('<%dQ' % (len(aggregatedTimeEntries)), byteArr1, 0))
        pass

    # Inset record to timingRecord
    for i in range(len(fileNameList)):
        record = FileRecord()
        record.fileName = fileNameList[i]
        timingRecord.append(record)

    for i in range(len(symbolNameList)):
        if symbolNameList[i] == 'pthread_create':
            # Attribute this time to pthread library
            realFileId[i] = pthreadFileId


    # Insert the time of the main application
    timingRecord[0].totalDuration = aggregatedTimeEntries[-1]

    # Loop through all timing entries and attribute time
    for i in range(len(aggregatedTimeEntries) - 1):
        # Add current symbol time to corresponding file entry
        if aggregatedTimeEntries[i] > 0:
            # Correct symbol ID
            if realFileId[i] >= len(fileNameList):
                print(
                    'Symbol %s was located in unhooked file. Is it overrided by an unhooked file?' % symbolNameList[
                        i])
                continue

            # Attribute time with respect to the caller
            curFileRecord = timingRecord[symbolFileIdList[i]]
            curFileRecord.fileName = fileNameList[symbolFileIdList[i]]
            curFileRecord.selfDuration -= aggregatedTimeEntries[i]
            curExtFileRecord = curFileRecord.extFileTiming[realFileId[i]]
            curExtFileRecord.fileName = fileNameList[realFileId[i]]
            curExtFileRecord.totalExtTime += aggregatedTimeEntries[i]
            curExtSymRecord = curExtFileRecord.extSymTiming[symIdInFile[i]]
            curExtSymRecord.symbolName = symbolNameList[i]
            curExtSymRecord.time = aggregatedTimeEntries[i]

            # Attribute time with respect to call-ee
            realFileRecord = timingRecord[realFileId[i]]
            print(realFileId[i])
            realFileRecord.fileName = fileNameList[realFileId[i]]
            realFileRecord.totalDuration += aggregatedTimeEntries[i]

    applicationDuration = timingRecord[0].totalDuration
    for i in range(len(timingRecord)):
        curFileRecord = timingRecord[i]
        curFileRecord.selfDuration += curFileRecord.totalDuration
        curFileRecord.selfDurationPerc0ent = curFileRecord.selfDuration / applicationDuration * 100
        for j in curFileRecord.extFileTiming.keys():
            curExtFileRecord = curFileRecord.extFileTiming[j]
            curExtFileRecord.totalExtTimePercent = curExtFileRecord.totalExtTime / curFileRecord.totalDuration * 100
            for k in curExtFileRecord.extSymTiming.keys():
                curExtSymRecord = curExtFileRecord.extSymTiming[k]
                curExtSymRecord.timePercent = curExtSymRecord.time / curExtFileRecord.totalExtTime * 100

    print(timingRecord)


def readTimingStruct(threadFileFullPath):
    timeEntries = None

    with open(threadFileFullPath, 'rb') as f:
        byteArr = f.read()
        mainFileId, timeEntrySize = struct.unpack_from('qq', byteArr, 0)  # 16 bytes
        timeEntries = struct.unpack_from('%dQ' % (timeEntrySize), byteArr, 16)
        assert (len(symbolNameList) == timeEntrySize - 1)
        assert (len(timeEntries) == timeEntrySize)
    return timeEntries


aggregatedTimeArray = None
for threadId in threadIdList:
    entryOfThisThread = readTimingStruct(os.path.join(scalerDataFolder, 'threadTiming_%s.bin' % threadId))
    if aggregatedTimeArray is None:
        aggregatedTimeArray = np.array(entryOfThisThread)
    else:
        aggregatedTimeArray = np.add(aggregatedTimeArray, entryOfThisThread)

# Generate graph
generateTimingStruct(list(aggregatedTimeArray))
