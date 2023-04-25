import os
import traceback
from collections import defaultdict

from matplotlib import cm
import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time

from util.Quantile import calcQuantile
from datastructure.TimingStruct import ArrayDescriptor
from util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from util.Parser.TimeOutputPrarser import readSymbolFiles, aggregatePerThreadArray, readTimingStruct
import numpy as np


def calcInvokedApiNum(scalerDataFolder, recInfo):
    invokedAPIs = []
    totalAPIs = []
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]
        invokedAPIs.append(len(curThreadInvokedRecArray))
        totalAPIs.append(len(curThreadRecArray))
    return invokedAPIs, totalAPIs


def calcInvokedApiCNT(scalerDataFolder, recInfo):
    invokedAPICnts = []
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        invokedAPICnts.append(np.sum([rec.count for rec in curThreadRecArray]))
    return invokedAPICnts


def printInvocNumberByEachThread(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return

    recInfo = readSymbolFiles(scalerDataFolder)
    invokedAPIs, totalAPIs = calcInvokedApiNum(scalerDataFolder, recInfo)

    totalInvocationCnts = 0

    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        totalInvocationCnts += np.sum(times)

    minimum, q1, q2, q3, q4, iqr, qLower, qUpper = calcQuantile(invokedAPIs)

    print(scalerDataFolder.split('/')[-3], minimum, q1, q2, q3, q4, iqr, qLower, qUpper, len(invokedAPIs), totalAPIs[0],
          totalInvocationCnts, sep='\t')


def printInvocCntByEachThread(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return

    recInfo = readSymbolFiles(scalerDataFolder)
    invokedAPIs = calcInvokedApiCNT(scalerDataFolder, recInfo)

    totalInvocationCnts = 0

    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        totalInvocationCnts += np.sum(times)

    minimum, q1, q2, q3, q4, iqr, qLower, qUpper = calcQuantile(invokedAPIs)

    print(scalerDataFolder.split('/')[-3], minimum, q1, q2, q3, q4, iqr, qLower, qUpper, totalInvocationCnts, sep='\t')


def printInvocCnt(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return

    recInfo = readSymbolFiles(scalerDataFolder)

    totalCountArr = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        if totalCountArr is None:
            totalCountArr = times
        else:
            totalCountArr = np.vstack([totalCountArr, times])

    if len(totalCountArr.shape)==2:
        # if totalCountArr.shape
        totalCountArr = np.array(totalCountArr).sum(axis=0)


    totalCountArr1 = totalCountArr[np.where(totalCountArr > 0)]

    if len(totalCountArr1)==0:
        print(scalerDataFolder.split('/')[-3], 0, 0, 0, 0, 0, 0, 0, 0, 0, sep='\t')
    else:
        minima, q1, q2, q3, q4, iqr, qLower, qUpper = calcQuantile(totalCountArr1)
        print(scalerDataFolder.split('/')[-3], minima, q1, q2, q3, q4, iqr, qLower, qUpper, np.sum(totalCountArr1), sep='\t')


class APIInfo:
    def __init__(self):
        self.name = None
        self.cntSum = []
        self.timeAvg = []
        self.timeVa = []
        self.timeAvgDenoise = None
        self.timeVarDenoise = None


def printPerAPIInfoAndCnts(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return
    print(scalerDataFolder.split('/')[-3])
    recInfo = readSymbolFiles(scalerDataFolder)

    totalCountArr = None
    totalVarianceArry = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)

        counts = np.array([rec.count for rec in curThreadRecArray])
        if totalCountArr is None:
            totalCountArr = counts
        else:
            totalCountArr = np.vstack([totalCountArr, counts])
    if len(totalCountArr.shape)==2:
        # if totalCountArr.shape
        totalCountArr = np.array(totalCountArr).sum(axis=0)

    sortedNameCntTuple = [
        (i, recInfo.symbolNameList[i], recInfo.symbolFileIdList[i], recInfo.fileNameList[recInfo.symbolFileIdList[i]],
         recInfo.realFileIdList[i], recInfo.fileNameList[recInfo.realFileIdList[i]], totalCountArr[i]) for i in
        range(totalCountArr.shape[0]) if
        totalCountArr[i] > 0]

    for symId, symName, invokerFIleId, invokerFileName, realFileId, realFileName, count in sorted(sortedNameCntTuple,
                                                                                                  reverse=True,
                                                                                                  key=lambda x: x[-1]):
        print(symId, symName, invokerFIleId, invokerFileName.split('/')[-1], realFileId, realFileName.split('/')[-1],
              count, sep='\t')


def printPerLibInfoAndCnts(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return
    print(scalerDataFolder.split('/')[-3])
    recInfo = readSymbolFiles(scalerDataFolder)

    totalCountArr = None
    totalVarianceArry = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)

        counts = np.array([rec.count for rec in curThreadRecArray])
        if totalCountArr is None:
            totalCountArr = counts
        else:
            totalCountArr = np.vstack([totalCountArr, counts])
    if len(totalCountArr.shape)==2:
        # if totalCountArr.shape
        totalCountArr = np.array(totalCountArr).sum(axis=0)

    sortedNameCntTuple = [
        (i, recInfo.symbolNameList[i], recInfo.symbolFileIdList[i], recInfo.fileNameList[recInfo.symbolFileIdList[i]],
         recInfo.realFileIdList[i], recInfo.fileNameList[recInfo.realFileIdList[i]], totalCountArr[i]) for i in
        range(totalCountArr.shape[0]) if
        totalCountArr[i] > 0]

    libFileDict = defaultdict(int)

    for symId, symName, invokerFIleId, invokerFileName, realFileId, realFileName, count in sorted(sortedNameCntTuple,
                                                                                                  reverse=True,
                                                                                                  key=lambda x: x[-1]):
        libFileDict[realFileName] += count

    countList = list(libFileDict.items())
    countList = sorted(countList, key=lambda x: x[0])
    for name, count in countList:
        print(name, count, sep='\t')


# steven@masslabserv1:~/Downloads/DistributionAnalysis$ find . -name "scalerdata*"
scalerDataFolders = [
                    '/home/steven/Downloads/Effectiveness/dedup-malloc/scalerdata_579655075034290']




print('Thread inovked API #')
for scalerDataFolder in scalerDataFolders:
    printInvocNumberByEachThread(scalerDataFolder)

print('Thread inovked API CNT')
for scalerDataFolder in scalerDataFolders:
    printInvocCntByEachThread(scalerDataFolder)

print('API invocation CNT Analysis')
for scalerDataFolder in scalerDataFolders:
    printInvocCnt(scalerDataFolder)

print('Per-API infos')
for scalerDataFolder in scalerDataFolders:
    printPerAPIInfoAndCnts(scalerDataFolder)

print('Per-Lib infos')
for scalerDataFolder in scalerDataFolders:
    printPerLibInfoAndCnts(scalerDataFolder)
