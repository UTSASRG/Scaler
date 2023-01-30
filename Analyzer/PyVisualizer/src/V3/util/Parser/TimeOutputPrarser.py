import math
import os
import re
import struct
from datastructure.TimingStruct import FileRecord, RecTuple, RecordingInfo, RecordingInfo, ArrayDescriptor, \
    ThreadCreatorInfo
import pandas as pd
from collections import defaultdict


def readSymbolFiles(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return
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


def readTimingStruct(ROOT_PATH, threadId):
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
    return recDataArr, threadCreatorInfo


# def readApiInvocTimeByLibStruct(ROOT_PATH, threadId):
#     selfTimeList = None
#     with open(os.path.join(ROOT_PATH, 'apiInvocTimeByLib_%s.bin' % threadId), 'rb') as f:
#         arrayDescriptor = ArrayDescriptor()
#         f.readinto(arrayDescriptor)
#         assert (arrayDescriptor._magicNum == 167)
#
#         selfTimeList = list(struct.unpack_from('<%dQ' % (arrayDescriptor.arraySize),
#                                                f.read(arrayDescriptor.arrayElemSize * arrayDescriptor.arraySize)))
#     # assert (len(symbolNameList) == recArrSize - 1)
#     return selfTimeList
#

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
    totalCount = 0

    aggregatedTimeArray = []
    aggregatedCreatorTime = defaultdict(
        lambda: 0)  # Map fileId and starting time. Thread may be created by modules other than the main application
    # aggregatedApiUnscaledInvcTimeByLib = []

    waitTimeByThread=defaultdict(lambda: 0)
    for threadId in recInfo.threadIdList:
        curThreadRecArray, threadCreatorInfo = readTimingStruct(scalerDataFolder, threadId)
        # curApiUnscaledInvcTimeByLib = readApiInvocTimeByLibStruct(scalerDataFolder, threadId)

        aggregatedCreatorTime[threadCreatorInfo.threadCreatorFileId] += threadCreatorInfo.threadExecutionCycles
        # print(curThreadRecArray[-1].totalClockCycles)

        for i, curRec in enumerate(curThreadRecArray):
            api += 1
            totalCount += curRec.count
            # if curRec.count>0:
            # print('totalCount',totalCount,curRec.count)

        for i, curRec in enumerate(curThreadRecArray):
            if  recInfo.symbolNameList[i] in ['pthread_cond_wait', 'pthread_barrier_wait', 'pthread_cond_timedwait']:
                waitTimeByThread[threadId]+=curRec.totalClockCycles

        if len(curThreadRecArray) != len(aggregatedTimeArray):
            # First time
            aggregatedTimeArray = curThreadRecArray.copy()
            # aggregatedApiUnscaledInvcTimeByLib = curApiUnscaledInvcTimeByLib.copy()
        else:
            for i, curRec in enumerate(curThreadRecArray):
                aggregatedTimeArray[i].count += curRec.count
                aggregatedTimeArray[i].totalClockCycles += curRec.totalClockCycles

            # for i, curTime in enumerate(curApiUnscaledInvcTimeByLib):
            #     aggregatedApiUnscaledInvcTimeByLib[i] += curTime

    print(waitTimeByThread)
    for key,val in waitTimeByThread.items():
        print('Thread Wait Imbalance',key,val,sep='\t')
    return aggregatedTimeArray, aggregatedCreatorTime


pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')
