import os
import traceback

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time


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


def saveSinglePlot(ROOT_PATH, symbolNameList, threadIdSet, symId):
    # hasPoints = False

    for i, threadId in enumerate(threadIdSet):
        with open(os.path.join(ROOT_PATH, 'threadDetailedTiming_%s_%d.bin' % (threadId, symId)),
                  'rb') as f:
            fig, (ax1, ax2) = plt.subplots(2)
            byteArr1 = f.read()
            elemSize = np.fromfile(os.path.join(ROOT_PATH, 'threadDetailedTiming_%s_%d.bin' % (threadId, symId)),
                                   np.int64, count=1, offset=0)[0]
            if elemSize == 0:
                continue
            symbolTiming = np.fromfile(
                os.path.join(ROOT_PATH, 'threadDetailedTiming_%s_%d.bin' % (threadId, symId)),
                np.int64, offset=8)
            skipThis = shouldSkip(symbolTiming)
            if skipThis:
                continue
            if symbolTiming.shape[0] < 1001:
                continue
            ax1.scatter(np.arange(elemSize), symbolTiming, s=10)
            # Calculate the first 500 mean
            mean = np.average(symbolTiming[0:500])
            meanUpperbound = mean * (1 + 0.01)
            meanLowerbound = mean * (1 - 0.01)

            # ax2.text(i*50, i * 20, str(np.var(symbolTiming[0:500])))
            ax2.scatter(np.arange(1000), symbolTiming[0:1000], s=10)
            ax2.hlines(meanUpperbound, 0, elemSize, colors='red')
            ax2.hlines(meanLowerbound, 0, elemSize, colors='red')
            # hasPoints = True

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
    threadIdSet = set({})
    maxSymId = 0
    for fileName in allFiles:
        if fileName.startswith('threadDetailedTiming') and fileName.endswith('.bin'):
            _, threadId, symbolId = fileName.replace('.bin', '').split('_')
            symbolId = int(symbolId)
            if symbolId > maxSymId:
                maxSymId = symbolId
            threadIdSet.add(threadId)

    df = pd.read_csv(os.path.join(ROOT_PATH, 'fileName.txt'))
    fileNameList = df['pathName'].to_list()

    df = pd.read_csv(os.path.join(ROOT_PATH, 'symbolInfo.txt'))
    symbolNameList = df['funcName'].to_list()
    symbolFileIdList = df['fileId'].to_list()
    symIdInFile = df['symIdInFile'].to_list()

    print('Deploying tasks to pool')
    # for symId in range(maxSymId):
    #     saveSinglePlot(ROOT_PATH, symbolNameList, threadIdSet, symId)
    for symId in range(maxSymId):
        res = pool.apply_async(saveSinglePlot, args=[ROOT_PATH, symbolNameList, threadIdSet, symId],
                               error_callback=error_callback)
        rltList.append(res)


pool = Pool(60)
rltList = []

for i in [
    'scalerdata_6364935512299934',
    'scalerdata_6364979105953714',
    'scalerdata_6365014036860570',
    'scalerdata_6365088124846144',
    'scalerdata_6365123879328866',
    'scalerdata_6365618607468352',
    'scalerdata_6365739459778370',
    'scalerdata_6365776935349298',
    'scalerdata_6365841128804326',
    'scalerdata_6366139523773026',
    'scalerdata_6366165053302622'
]:
    ROOT_PATH = '/media/umass/datasystem/steven/Downloads/CurStrategy1/' + i
    doIt(ROOT_PATH, pool, rltList)
pool.close()
while len(rltList) > 0:
    time.sleep(2)
    rltList = [rlt for rlt in rltList if not rlt.ready()]
    print("%d jobs left" % len(rltList))
pool.join()
