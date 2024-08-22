'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
import math
import os
import re
import struct
from Analyzer.PyVisualizer.src.V3.datastructure.TimingStruct import FileRecord, RecTuple, RecordingInfo, RecordingInfo, \
    ArrayDescriptor, \
    ThreadCreatorInfo
import pandas as pd
from collections import defaultdict


def readSymbolFiles(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return
    rlt = RecordingInfo()

    with open(os.path.join(scalerDataFolder, 'fileName.txt'), 'r') as f:
        rlt.fileNameList = [line[:-1] for line in f.readlines()]
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
    df = pd.read_csv(os.path.join(scalerDataFolder, 'symbolInfo.txt'), dtype={
        'funcName': 'object',
        'globalFileId': 'int',
        'symIdInFile': 'int'
    })
    rlt.symbolNameList = df['funcName'].to_list()
    rlt.symbolFileIdList = df['globalFileId'].to_list()
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

class ThreadWaitAndExecutionRecord:
    def __init__(self):
        self.threadName = ''
        self.waitTime = 0
        self.joinTime = 0
        self.totalExecutionTime = 0


def adjustThreadCreatorInfo(recInfo: RecordingInfo):
    '''
    Some libc APIs may contains in
    '''
    pthreadFileId=0
    for fileId,fileName in enumerate(recInfo.fileNameList):
        fileName = fileName.split(os.sep)[-1]
        if fileName.startswith('libpthread'):
            pthreadFileId=fileId
            break
    libcFileId=0
    for fileId,fileName in enumerate(recInfo.fileNameList):
        fileName = fileName.split(os.sep)[-1]
        if fileName.startswith('libc-'):
            libcFileId=fileId
            break
    #Attribute non-pthread function to libc
    for symbolId in range(len(recInfo.symbolNameList)):
        if recInfo.realFileIdList[symbolId] == pthreadFileId and not recInfo.symbolNameList[symbolId].startswith('pthread_'):
            print('Adjust API %s:%s to %s'%(recInfo.fileNameList[pthreadFileId].split(os.sep)[-1],
                                            recInfo.symbolNameList[symbolId],
                                            recInfo.fileNameList[libcFileId].split(os.sep)[-1]))
            # Start
            recInfo.realFileIdList[symbolId]=libcFileId
    return recInfo
    # Find pthread fileId


def aggregatePerThreadArray(scalerDataFolder, recInfo: RecordingInfo, returnThreadImbalance=False):
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

    perThreadWaitAndExecutionTime = defaultdict(ThreadWaitAndExecutionRecord)
    aggregatedTimeArray = []
    aggregatedCreatorTime = defaultdict(lambda: 0)

    for threadId in recInfo.threadIdList:
        curThreadRecArray, threadCreatorInfo = readTimingStruct(scalerDataFolder, threadId)
        # curApiUnscaledInvcTimeByLib = readApiInvocTimeByLibStukvjigciggnngercrngjuruct(scalerDataFolder, threadId)
        # print('threadCreatorInfo.threadExecutionCyclcccbhhvtbes',threadCreatorInfo.threadExecutionCycles)
        # print(threadCreatorInfo.threadCreatorFileId)
        curWaitAndExecutionTime = perThreadWaitAndExecutionTime[threadId]
        curWaitAndExecutionTime.totalExecutionTime = threadCreatorInfo.threadExecutionCycles
        curWaitAndExecutionTime.threadName = str(threadCreatorInfo.threadName, 'UTF-8')
        aggregatedCreatorTime[threadCreatorInfo.threadCreatorFileId] += threadCreatorInfo.threadExecutionCycles

        # print(curThreadRecArray[-1].totalClockCycles)

        for i, curRec in enumerate(curThreadRecArray):
            assert (curRec.totalClockCycles < threadCreatorInfo.threadExecutionCycles)

        for i, curRec in enumerate(curThreadRecArray):
            api += 1
            totalCount += curRec.count
            # if curRec.count>0:
            # print('totalCount',totalCount,curRec.count)

        for i, curRec in enumerate(curThreadRecArray):
            if recInfo.symbolNameList[i] in ['pthread_cond_wait', 'pthread_barrier_wait', 'pthread_cond_timedwait']:
                perThreadWaitAndExecutionTime[threadId].waitTime += curRec.totalClockCycles
            if recInfo.symbolNameList[i] in ['pthread_join']:
                perThreadWaitAndExecutionTime[threadId].joinTime += curRec.totalClockCycles

        if len(curThreadRecArray) != len(aggregatedTimeArray):
            # First time
            aggregatedTimeArray = curThreadRecArray.copy()
            # aggregatedApiUnscaledInvcTimeByLib = curApiUnscaledInvcTimeByLib.copy()
        else:
            # Expand aggregatedTimeArray if we find incoming array is larger
            if len(aggregatedTimeArray) < len(curThreadRecArray):
                aggregatedTimeArray.extend(curThreadRecArray[len(aggregatedTimeArray):])

            for i, curRec in enumerate(curThreadRecArray):
                aggregatedTimeArray[i].count += curRec.count
                aggregatedTimeArray[i].totalClockCycles += curRec.totalClockCycles

            # for i, curTime in enumerate(curApiUnscaledInvcTimeByLib):
            #     aggregatedApiUnscaledInvcTimeByLib[i] += curTime

    # print(waitTimeByThread)
    # for key, val in waitTimeByThread.items():
    #     print(val[0], key, val[1], sep='\t')
    if returnThreadImbalance:
        return aggregatedTimeArray, aggregatedCreatorTime, perThreadWaitAndExecutionTime
    else:
        return aggregatedTimeArray, aggregatedCreatorTime


pthreadFileRegex = re.compile(r'libpthread-.*\.so$')


def parsePthreadId(fileNameList):
    for i, fileName in enumerate(fileNameList):
        if len(pthreadFileRegex.findall(fileName)) != 0:
            return i
    raise Exception('Cannot find pthread library in fileList')


def plotThreadImbalance(scalerDataFolder, recInfo: RecordingInfo):
    api = 0
    totalCount = 0
