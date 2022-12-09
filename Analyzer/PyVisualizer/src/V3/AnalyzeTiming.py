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

from Analyzer.PyVisualizer.src.V3.datastructure.Metric import Metric
from Analyzer.PyVisualizer.src.V3.util.Quantile import calcQuantile
from datastructure.TimingStruct import ArrayDescriptor
from util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from util.Parser.TimeOutputPrarser import readSymbolFiles, aggregatePerThreadArray, readTimingStruct
import numpy as np


def parsePerLibSelfTime(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return
    recInfo = readSymbolFiles(scalerDataFolder)

    totalTimingArr = None
    totalVarianceArry = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder, threadId)

        cycles = np.array([rec.totalClockCycles for rec in curThreadRecArray])
        if totalTimingArr is None:
            totalTimingArr = cycles
        else:
            totalTimingArr = np.vstack([totalTimingArr, cycles])

    if len(totalTimingArr.shape) == 1:
        totalTimingArr = np.reshape(totalTimingArr, (1, -1))

    sortedNameCntTuple = []

    for i in range(totalTimingArr.shape[1] - 1):
        if np.sum(totalTimingArr[:, i] > 0):
            sortedNameCntTuple.append((i, recInfo.symbolNameList[i], recInfo.symbolFileIdList[i],
                                       recInfo.fileNameList[recInfo.symbolFileIdList[i]],
                                       recInfo.realFileIdList[i], recInfo.fileNameList[recInfo.realFileIdList[i]],
                                       np.sum(totalTimingArr[:, i])))

    libFileDict = defaultdict(int)

    libFileDict = defaultdict(int)

    for symId, symName, invokerFIleId, invokerFileName, realFileId, realFileName, time in sorted(sortedNameCntTuple,
                                                                                                 reverse=True,
                                                                                                 key=lambda x: x[-1]):
        libFileDict[realFileName] += time

    # timeList = list(libFileDict.items())
    # timeList = sorted(timeList, key=lambda x: x[0])
    #
    # for name, time in timeList:
    #     print(name, time, sep='\t')
    return libFileDict


def findScalerDataFolder(rootPath):
    for root, dirs, files in os.walk(rootPath):
        for dir in dirs:
            if dir.startswith('scalerdata'):
                return os.path.join(root, dir)


scalerDataFolders = '/media/umass/datasystem/steven/Downloads/accuracyTest/2022-12-08_06-26-18-Sampling-0B111'
print('Per-Lib infos')

pathDict = defaultdict(list)

for folderName in os.listdir(scalerDataFolders):
    cache= folderName.split('.')[-1]
    appName=cache[0:-2]
    runTime=cache[-1:]
    scalerDataFolder = findScalerDataFolder(os.path.join(scalerDataFolders, folderName))
    pathDict[appName].append(scalerDataFolder)

rlt = {}
for appName, pathList in pathDict.items():
    rlt[appName] = defaultdict(Metric)
    for path in pathList:
        libFileDict = parsePerLibSelfTime(path)
        for libName, time in libFileDict.items():
            rlt[appName][libName].append(time)

# Sort Results
rlt = list(rlt.items())
rlt = sorted(rlt, key=lambda x: x[0])
for appName, libraryList in rlt:
    libraryList = list(libraryList.items())
    libraryList = sorted(libraryList, key=lambda x: x[0])
    print(appName)
    for libraryName, metric in libraryList:
        print(libraryName, metric.mean(), metric.std(), metric.std() / metric.mean(), sep='\t')
