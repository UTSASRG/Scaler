import os
import re
import struct
from datastructure.TimingStruct import FileRecord, RecTuple, RecordingInfo, RecordingInfo
import pandas as pd
from collections import defaultdict


def readSymbolFiles(scalerDataFolder):
    rlt = RecordingInfo()

    df = pd.read_csv(os.path.join(scalerDataFolder, 'fileName.txt'))
    rlt.fileNameList = df['pathName'].to_list()
    rlt.pthreadFileId = parsePthreadId(rlt.fileNameList)

    for fileName in os.listdir(scalerDataFolder):
        if fileName.startswith('threadTiming_'):
            rlt.threadIdList.append(fileName[len('threadTiming_'): -4])

    with open(os.path.join(scalerDataFolder, 'realFileId.bin'), 'rb') as f:
        byteArr1 = f.read()
        arraySize = struct.unpack_from('Q', byteArr1, 0)  # The first element is the array size
        rlt.realFileIdList = list(struct.unpack_from('<%dQ' % (arraySize), byteArr1,
                                                     8))  # The last id marks the creator thread
    df = pd.read_csv(os.path.join(scalerDataFolder, 'symbolInfo.txt'))
    rlt.symbolNameList = df['funcName'].to_list()
    rlt.symbolFileIdList = df['fileId'].to_list()
    rlt.symIdInFileList = df['symIdInFile'].to_list()
    return rlt


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
    # assert (len(symbolNameList) == recArrSize - 1)
    assert (len(recDataArr) == recArrSize)
    return recDataArr


def aggregatePerThreadArray(scalerDataFolder, recInfo: RecordingInfo):
    """
    - Aggregate per-thread timing data into one using simple addition and return as the first return value
    - The last element in each RecTuple records how much time the thread takes to execute so we should not aggregate them
    together. Instead, we collect them into one list and return as the second parameter

    :param scalerDataFolder: Scaler output data folder
    :param threadIdList: A list of thread ids
    :return aggregatedTimeArray: Aggregated counting and timing information
    :return startingInfoArray: Information about thread creator. This value is used in time aggregation steps
    """

    aggregatedTimeArray = []
    aggregatedStartingTime = defaultdict(
        lambda: 0)  # Map fileId and starting time. Thread may created by modules other than the main application
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(os.path.join(scalerDataFolder, 'threadTiming_%s.bin' % threadId))
        aggregatedStartingTime[curThreadRecArray[-1]._flags] += curThreadRecArray[-1].totalClockCycles
        print(curThreadRecArray[-1].totalClockCycles)

        if len(curThreadRecArray) != len(aggregatedTimeArray) + 1:
            # First time
            aggregatedTimeArray = curThreadRecArray[:-1].copy()
        else:
            for i, curRec in enumerate(curThreadRecArray[:-1]):
                aggregatedTimeArray[i].count += curRec.count
                if recInfo.symbolNameList[i] == 'pthread_join':
                    print('Skip pthread_join')
                    continue

                if aggregatedTimeArray[i]._flags & (1 << 0):
                    # Use mean and count to estimate total clock cycles
                    aggregatedTimeArray[i].totalClockCycles += int(curRec.count * curRec._meanClockTick)
                else:
                    aggregatedTimeArray[i].totalClockCycles += curRec.totalClockCycles

    return aggregatedTimeArray, aggregatedStartingTime


pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')


def generateTimingStruct(aggregatedTimeEntries, aggregatedStartingTime, recInfo: RecordingInfo):
    timingRecord = []  # Map file name to FileRecord struct
    mainFileId = None

    # Inset record to timingRecord
    for i in range(len(recInfo.fileNameList)):
        record = FileRecord()
        record.fileName = recInfo.fileNameList[i]
        timingRecord.append(record)

    # Attribute pthread_create to pthread library
    for i in range(len(recInfo.symbolNameList)):
        if recInfo.symbolNameList[i] == 'pthread_create':
            # Attribute this time to pthread library rather than scaler
            recInfo.realFileIdList[i] = recInfo.pthreadFileId

    totalProgramRunningTIme = 0
    totalInvocationCount = 0

    # Attribute the total thread running time
    for fileId, totalClockCycles in aggregatedStartingTime.items():
        timingRecord[fileId].selfClockCycles.value = totalClockCycles
        totalProgramRunningTIme += totalClockCycles

    # Loop through all timing entries and attribute time
    for i in range(len(aggregatedTimeEntries)):
        # Add current symbol time to corresponding file entry (Only record APIs that are invoked at least once)
        if aggregatedTimeEntries[i].count > 0:
            # Make sure symbol ID is correct
            if recInfo.realFileIdList[i] >= len(recInfo.fileNameList):
                print(
                    'Symbol %s was located in unhooked file. Is it overrided by an unhooked file?' %
                    recInfo.symbolNameList[
                        i])
                continue

            # Attribute time to callee
            curFileRecord = timingRecord[recInfo.symbolFileIdList[i]]
            curFileRecord.fileName = recInfo.fileNameList[recInfo.symbolFileIdList[i]]
            curFileRecord.selfClockCycles.value -= aggregatedTimeEntries[i].totalClockCycles
            curFileRecord.childrenClockCycles.value += aggregatedTimeEntries[i].totalClockCycles

            curExtFileRecord = curFileRecord.extFileTiming[recInfo.realFileIdList[i]]
            curExtFileRecord.fileName = recInfo.fileNameList[recInfo.realFileIdList[i]]
            curExtFileRecord.totalClockCycles.value += aggregatedTimeEntries[i].totalClockCycles
            curExtFileRecord.counts.value += aggregatedTimeEntries[i].count

            curExtSymRecord = curExtFileRecord.extSymTiming[recInfo.symIdInFileList[i]]
            curExtSymRecord.symbolName = recInfo.symbolNameList[i]
            curExtSymRecord.totalClockCycles.value = aggregatedTimeEntries[i].totalClockCycles
            curExtSymRecord.counts.value = aggregatedTimeEntries[i].count

            # Attribute time to caller
            realFileRecord = timingRecord[recInfo.realFileIdList[i]]
            # realFileRecord.fileName = fileNameList[recInfo.realFileIdList[i]]
            realFileRecord.selfClockCycles.value += aggregatedTimeEntries[i].totalClockCycles

            totalInvocationCount += aggregatedTimeEntries[i].count

    timingRecord = calcPercentage(timingRecord, totalProgramRunningTIme, totalInvocationCount)
    return timingRecord


def calcPercentage(timingRecord, programRuntime, totalApiCallCount):
    for curFileRecord in timingRecord:
        for curExtFileRecord in curFileRecord.extFileTiming.values():
            for curExtSymRecord in curExtFileRecord.extSymTiming.values():
                if curExtFileRecord.counts.value > 0:
                    curExtSymRecord.counts.parentPercent = curExtSymRecord.counts.value / curExtFileRecord.counts.value
                else:
                    curExtSymRecord.counts.parentPercent = 0.0
                if totalApiCallCount > 0:
                    curExtSymRecord.counts.globalPercent = curExtSymRecord.counts.value / totalApiCallCount
                else:
                    curExtSymRecord.counts.globalPercent = 0.0

                if curExtSymRecord.totalClockCycles.value > 0:
                    curExtSymRecord.totalClockCycles.parentPercent = curExtSymRecord.totalClockCycles.value / curExtFileRecord.totalClockCycles.value
                else:
                    curExtSymRecord.totalClockCycles.parentPercent = 0.0

                if programRuntime > 0:
                    curExtSymRecord.totalClockCycles.globalPercent = curExtSymRecord.totalClockCycles.value / programRuntime
                else:
                    curExtSymRecord.totalClockCycles.globalPercent = 0.0

            if totalApiCallCount > 0:
                curExtFileRecord.counts.globalPercent = curExtFileRecord.counts.value / totalApiCallCount
            else:
                curExtFileRecord.counts.globalPercent = 0.0

            if curFileRecord.childrenClockCycles.value > 0:
                curExtFileRecord.totalClockCycles.parentPercent = curExtFileRecord.totalClockCycles.value / (
                            curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
            else:
                curExtFileRecord.totalClockCycles.parentPercent = 0.0

            if programRuntime > 0:
                curExtFileRecord.totalClockCycles.globalPercent = curExtFileRecord.totalClockCycles.value / programRuntime
            else:
                curExtFileRecord.totalClockCycles.globalPercent = 0.0

        if curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value > 0:
            curFileRecord.selfClockCycles.totalPercent = curFileRecord.selfClockCycles.value / (
                    curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
        else:
            curFileRecord.selfClockCycles.totalPercent = 0.0

        if programRuntime > 0:
            curFileRecord.selfClockCycles.globalPercent = curFileRecord.selfClockCycles.value / programRuntime
        else:
            curFileRecord.selfClockCycles.globalPercent = 0.0

        if curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value > 0:
            curFileRecord.childrenClockCycles.totalPercent = curFileRecord.childrenClockCycles.value / (
                    curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
        else:
            curFileRecord.childrenClockCycles.totalPercent = 0.0

        if programRuntime > 0:
            curFileRecord.childrenClockCycles.globalPercent = curFileRecord.childrenClockCycles.value / programRuntime
        else:
            curFileRecord.childrenClockCycles.globalPercent = 0.0

    return timingRecord
