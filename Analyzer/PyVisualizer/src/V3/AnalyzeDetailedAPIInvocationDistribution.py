import os
import shutil
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


def saveSinglePlot(ROOT_PATH, symbolNameList, threadId, tgtSymId, graphType='hist'):
    # hasPoints = False
    # ROOT_PATH: str, symbolNameList: list, threadId: str, tgtSymIds: list):
    detailedTimingDict = parseSingleSymDetailedTiming(ROOT_PATH, threadId, [tgtSymId])

    for symId, detailedTimingArr in detailedTimingDict.items():
        fig, ax1 = plt.subplots(1)

        # if detailedTimingArr.shape[0] < 1001:
        #     continue
        #
        # skipThis = shouldSkip(detailedTimingArr)
        # if skipThis:
        #     continue
        if detailedTimingArr.shape[0] > 0:
            if detailedTimingArr.max() < 1:
                continue

            if graphType == 'hist':
                ax1.hist(detailedTimingArr, range=(1, detailedTimingArr.max()), edgecolor="black", bins=50)
                # print(os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
                # if hasPoints:
            elif graphType == 'scatter':
                ax1.scatter(np.arange(detailedTimingArr.shape[0]), detailedTimingArr, s=10)
            else:
                assert (False)

            if not os.path.exists(os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId])):
                os.makedirs(os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId]), exist_ok=True)
            fig.savefig(
                os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId],
                             'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
            # print(os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
        plt.close(fig)
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
                res = pool.apply_async(saveSinglePlot,
                                       args=[ROOT_PATH, recInfo.symbolNameList, threadId, symId, 'hist'],
                                       error_callback=error_callback)
                rltList.append(res)

    return rltList


pool = Pool(79)
rltList = []

for ROOT_PATH in ['/media/umass/datasystem/steven/Downloads/Detailed_Timing/blackscholes',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/bodytrack',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/facesim',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/ferret',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/fluidanimate',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/freqmine',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/raytrace',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/swaptions',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/vips',
                  '/media/umass/datasystem/steven/Downloads/Detailed_Timing/x264'
                  ]:
    recInfo = readSymbolFiles(ROOT_PATH)
    doIt(ROOT_PATH, pool, rltList)

pool.close()
while len(rltList) > 0:
    time.sleep(2)
    rltList = [rlt for rlt in rltList if not rlt.ready()]
    print("%d jobs left" % len(rltList))
pool.join()
