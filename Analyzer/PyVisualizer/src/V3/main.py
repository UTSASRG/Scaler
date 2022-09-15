import os
import pandas as pd
from struct import Struct
import struct
from typing import List
import numpy as np
import re
from TimingStruct import FileRecord, ExtSymRecord, RecTuple

pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')


# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/media/umass/datasystem/steven/Downloads/scalerdata_6438774701359890'
# '/media/umass/datasystem/steven/Downloads/Perf-Parsec-Callgraph/ferret/scalerdata'

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


def generateTimingStruct(aggregatedTimeEntries):
    timingRecord = []  # Map file name to FileRecord struct
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
            # Attribute this time to pthread library rather than scaler
            realFileId[i] = pthreadFileId

    # Insert the time of the main application
    timingRecord[0].clockCycles.value = aggregatedTimeEntries[-1].totalClockCycles

    # Loop through all timing entries and attribute time
    for i in range(len(aggregatedTimeEntries) - 1):
        # Add current symbol time to corresponding file entry
        if aggregatedTimeEntries[i].count > 0:
            # Make sure symbol ID is correct
            if realFileId[i] >= len(fileNameList):
                print(
                    'Symbol %s was located in unhooked file. Is it overrided by an unhooked file?' % symbolNameList[
                        i])
                continue

            # Attribute time with respect to the caller
            curFileRecord = timingRecord[symbolFileIdList[i]]
            curFileRecord.fileName = fileNameList[symbolFileIdList[i]]
            curFileRecord.clockCycles.value -= aggregatedTimeEntries[i].totalClockCycles

            curExtFileRecord = curFileRecord.extFileTiming[realFileId[i]]
            curExtFileRecord.fileName = fileNameList[realFileId[i]]
            curExtFileRecord.totalClockCycles += aggregatedTimeEntries[i].totalClockCycles

            curExtSymRecord = curExtFileRecord.extSymTiming[symIdInFile[i]]
            curExtSymRecord.symbolName = symbolNameList[i]
            curExtSymRecord.totalClockCycles = aggregatedTimeEntries[i].totalClockCycles

            # Attribute time with respect to call-ee
            realFileRecord = timingRecord[realFileId[i]]
            print(realFileId[i])
            # realFileRecord.fileName = fileNameList[realFileId[i]]
            realFileRecord.totalClockCycles += aggregatedTimeEntries[i].totalClockCycles

    applicationDuration = timingRecord[0].totalClockCycles
    for i in range(len(timingRecord)):
        curFileRecord = timingRecord[i]
        curFileRecord.selfClockCycles += curFileRecord.totalClockCycles
        curFileRecord.selfDurationPerc0ent = curFileRecord.selfClockCycles / applicationDuration * 100
        for j in curFileRecord.extFileTiming.keys():
            curExtFileRecord = curFileRecord.extFileTiming[j]
            curExtFileRecord.totalExtTimePercent = curExtFileRecord.totalClockCycles / applicationDuration * 100
            for k in curExtFileRecord.extSymTiming.keys():
                curExtSymRecord = curExtFileRecord.extSymTiming[k]
                curExtSymRecord.timePercent = curExtSymRecord.totalClockCycles / applicationDuration * 100

    print(timingRecord)


def readTimingStruct(threadFileFullPath):
    recDataArr = []
    recTupleSize = 8 + 8 + 4 + 4 + 4 + 4
    with open(threadFileFullPath, 'rb') as f:
        byteArr = f.read()
        mainFileId, recArrSize = struct.unpack_from('qq', byteArr, 0)  # 16 bytes
        f.seek(16)

        for i in range(recArrSize):
            curRecFormat = RecTuple()
            f.readinto(curRecFormat)
            recDataArr.append(curRecFormat)
    assert (len(symbolNameList) == recArrSize - 1)
    assert (len(recDataArr) == recArrSize)
    return recDataArr


aggregatedTimeArray = []
for threadId in threadIdList:
    curThreadRecArray = readTimingStruct(os.path.join(scalerDataFolder, 'threadTiming_%s.bin' % threadId))
    if len(curThreadRecArray) != len(aggregatedTimeArray):
        aggregatedTimeArray = curThreadRecArray.copy()
    else:
        for i, curRec in curThreadRecArray:
            aggregatedTimeArray[i].totalClockCycles += curRec.totalClockCycles
            aggregatedTimeArray[i].count += curRec.count
            aggregatedTimeArray[i].gap += curRec.gap
            aggregatedTimeArray[i].meanDuration += curRec.meanDuration
            aggregatedTimeArray[i].durThreshold += curRec.durThreshold
            aggregatedTimeArray[i].flags |= curRec.flags

# Generate graph
generateTimingStruct(list(aggregatedTimeArray))
