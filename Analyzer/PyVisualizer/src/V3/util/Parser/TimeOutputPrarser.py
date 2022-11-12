import math
import os
import re
import struct
from datastructure.TimingStruct import FileRecord, RecTuple, RecordingInfo, RecordingInfo, ArrayDescriptor, \
    ThreadCreatorInfo
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

        arrDesc = ArrayDescriptor()
        f.readinto(arrDesc)
        assert (arrDesc._magicNum == 167)
        assert (arrDesc.arrayElemSize == 8)
        rlt.realFileIdList = list(
            struct.unpack_from('<%dQ' % (arrDesc.arraySize), f.read(arrDesc.arrayElemSize * arrDesc.arraySize)))
        assert (f.read() == b'')  # Make sure this is the end
    df = pd.read_csv(os.path.join(scalerDataFolder, 'symbolInfo.txt'))
    rlt.symbolNameList = df['funcName'].to_list()
    rlt.symbolFileIdList = df['fileId'].to_list()
    rlt.symIdInFileList = df['symIdInFile'].to_list()
    return rlt


def readTimingStruct(ROOT_PATH,threadId):
    recDataArr = []

    with open(os.path.join(ROOT_PATH, 'threadTiming_%s.bin' % threadId), 'rb') as f:
        threadCreatorInfo = ThreadCreatorInfo()
        f.readinto(threadCreatorInfo)
        assert (threadCreatorInfo._magicNum == 167)

        arrayDescriptor = ArrayDescriptor()
        f.readinto(arrayDescriptor)
        assert (arrayDescriptor._magicNum == 167)

        for i in range(arrayDescriptor.arraySize):
            curRecFormat = RecTuple()
            f.readinto(curRecFormat)
            recDataArr.append(curRecFormat)

    # assert (len(symbolNameList) == recArrSize - 1)
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
    api = 0
    fgdsApi = 0
    fgdsCount = 0
    totalCount = 0

    aggregatedTimeArray = []
    aggregatedStartingTime = defaultdict(
        lambda: 0)  # Map fileId and starting time. Thread may created by modules other than the main application
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder,threadId)
        aggregatedStartingTime[curThreadRecArray[-1]._flags] += curThreadRecArray[-1].totalClockCycles
        # print(curThreadRecArray[-1].totalClockCycles)

        for i, curRec in enumerate(curThreadRecArray[:-1]):
            if curRec._flags & (1 << 0):
                fgdsApi += 1
                fgdsCount += curRec.count
            api += 1
            totalCount += curRec.count
            # if curRec.count>0:
            # print('totalCount',totalCount,curRec.count)
        if len(curThreadRecArray) != len(aggregatedTimeArray) + 1:
            # First time
            aggregatedTimeArray = curThreadRecArray[:-1].copy()
        else:
            for i, curRec in enumerate(curThreadRecArray[:-1]):
                aggregatedTimeArray[i].count += curRec.count
                # if recInfo.symbolNameList[i] == 'pthread_join':
                #     print('Skip pthread_join')
                #     continue

                if aggregatedTimeArray[i]._flags & (1 << 0):
                    # Use mean and count to estimate total clock cycles
                    aggregatedTimeArray[i].totalClockCycles += int(curRec.count * curRec._meanClockTick)
                else:
                    aggregatedTimeArray[i].totalClockCycles += curRec.totalClockCycles
    print('fgdsapi/api=', round(fgdsApi / api * 100, 2), 'fgdsCount/TotalCount=',
          round(fgdsCount / totalCount * 100, 2), sep='\t')
    return aggregatedTimeArray, aggregatedStartingTime


pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')
