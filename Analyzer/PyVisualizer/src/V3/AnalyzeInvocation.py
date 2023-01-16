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

from Analyzer.PyVisualizer.src.V3.util.Quantile import calcQuantile
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
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.blackscholes_0/Scaler-DETAIL-Artifects/scalerdata_1098304170115468',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.bodytrack_0/Scaler-DETAIL-Artifects/scalerdata_1098358945736648',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.canneal_0/Scaler-DETAIL-Artifects/scalerdata_1098456760642602',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.dedup_0/Scaler-DETAIL-Artifects/scalerdata_1098609218045480',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.facesim_0/Scaler-DETAIL-Artifects/scalerdata_1098633517549600',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.ferret_0/Scaler-DETAIL-Artifects/scalerdata_1098896081465298',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.fluidanimate_0/Scaler-DETAIL-Artifects/scalerdata_1107220283374448',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.freqmine_0/Scaler-DETAIL-Artifects/scalerdata_1099112583217984',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.raytrace_0/Scaler-DETAIL-Artifects/scalerdata_1099182362951380',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.streamcluster_0/Scaler-DETAIL-Artifects/scalerdata_1099698097420154',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.swaptions_0/Scaler-DETAIL-Artifects/scalerdata_1099956187630596',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.vips_0/Scaler-DETAIL-Artifects/scalerdata_1100002948879490',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.x264_0/Scaler-DETAIL-Artifects/scalerdata_1100026471754668',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.webserver.httpd.httpd_2_4_54_0/Scaler-DETAIL-Artifects/scalerdata_1100053088973938',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.memcached.memcached_1_6_17_0/Scaler-DETAIL-Artifects/scalerdata_1100215829624386',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.mysql.mysql_8_0_31_0/Scaler-DETAIL-Artifects/scalerdata_1100366950684482',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.webserver.nginx.nginx_1_23_2_0/Scaler-DETAIL-Artifects/scalerdata_1100701228337962',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.redis.redis_7_0_4_0/Scaler-DETAIL-Artifects/scalerdata_1100850174945384',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.sqlite.sqlite_3_39_4_0/Scaler-DETAIL-Artifects/scalerdata_1101150204449494'
]

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
