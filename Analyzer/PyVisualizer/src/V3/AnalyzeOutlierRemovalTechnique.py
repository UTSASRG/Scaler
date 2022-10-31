import os
import traceback

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time

from Analyzer.PyVisualizer.src.V3.datastructure.TimingStruct import ArrayDescriptor
from Analyzer.PyVisualizer.src.V3.util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from Analyzer.PyVisualizer.src.V3.util.Parser.TimeOutputPrarser import readSymbolFiles


def removeOutliers(x):
    a = np.array(x)
    upper_quartile = np.percentile(a, 99)
    lower_quartile = np.percentile(a, 1)
    x[np.hstack([np.where(x >= upper_quartile)[0], np.where(x <= lower_quartile)[0]])] = 0


def shouldSkip(timingArr):
    if timingArr.shape[0] < 500:
        return False
    mean = np.average(timingArr[0:500])
    meanUpperbound = mean * (1 + 0.5)
    meanLowerbound = mean * (1 - 0.5)

    c1 = np.where(timingArr[500:1000] > meanUpperbound)[0].shape[0]
    c2 = np.where(timingArr[500:1000] < meanLowerbound)[0].shape[0]
    return c1 == 0 and c2 == 0


def saveSinglePlot(ROOT_PATH, symbolNameList, threadId, tgtSymId):
    # hasPoints = False
    # ROOT_PATH: str, symbolNameList: list, threadId: str, tgtSymIds: list):
    detailedTimingDict = parseSingleSymDetailedTiming(ROOT_PATH, threadId, [tgtSymId])

    for symId, detailedTimingArr in detailedTimingDict.items():
        fig, (ax1, ax2) = plt.subplots(2)

        # if detailedTimingArr.shape[0] < 1001:
        #     continue
        #
        # skipThis = shouldSkip(detailedTimingArr)
        # if skipThis:
        #     continue

        ax1.scatter(np.arange(detailedTimingArr.shape[0]), detailedTimingArr, s=10)
        # Calculate the first 500 mean
        mean = np.average(detailedTimingArr[0:500])
        meanUpperbound = mean * (1 + 0.01)
        meanLowerbound = mean * (1 - 0.01)

        ax2.scatter(np.arange(min(1000, detailedTimingArr.shape[0])),
                    detailedTimingArr[0:min(1000, detailedTimingArr.shape[0])], s=10)
        ax2.hlines(meanUpperbound, 0, detailedTimingArr.shape[0], colors='red')
        ax2.hlines(meanLowerbound, 0, detailedTimingArr.shape[0], colors='red')
        hasPoints = True

        print(os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
        # if hasPoints:
        fig.savefig(
            os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
        print(
            os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))

    return 0


def error_callback(e):
    print('error')
    print(dir(e), "\n")
    print("-->{}<--".format(e.__cause__))


def doIt(ROOT_PATH, pool, rltList):
    print('========================', ROOT_PATH)
    allFiles = os.listdir(ROOT_PATH)
    symbolNum = 0

    recInfo = readSymbolFiles(ROOT_PATH)
    threadSymInfo = dict({})  # Threadid : symbol size
    for fileName in allFiles:
        if fileName.startswith('threadDetailedTiming') and fileName.endswith('.bin'):
            _, threadId = fileName.replace('.bin', '').split('_')
            with open(os.path.join(ROOT_PATH, fileName), 'rb') as f:
                symDetailedTimingDesc = ArrayDescriptor()
                f.readinto(symDetailedTimingDesc)
                assert (symDetailedTimingDesc.arrayElemSize == 0)
                assert (symDetailedTimingDesc._magicNum == 167)
                symbolNum = symDetailedTimingDesc.arraySize
                threadSymInfo[threadId] = symbolNum

            for symId in range(symbolNum):
                res = pool.apply_async(saveSinglePlot, args=[ROOT_PATH, recInfo.symbolNameList, threadId, symId],
                                       error_callback=error_callback)
                rltList.append(res)

    return rltList


pool = Pool(1)
rltList = []

for ROOT_PATH in ['/tmp/scalerdata_14676207526291652']:
    doIt(ROOT_PATH, pool, rltList)

pool.close()
while len(rltList) > 0:
    time.sleep(2)
    rltList = [rlt for rlt in rltList if not rlt.ready()]
    print("%d jobs left" % len(rltList))
pool.join()
