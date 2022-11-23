import os
import traceback
from matplotlib import cm
import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time

from datastructure.TimingStruct import ArrayDescriptor
from util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from util.Parser.TimeOutputPrarser import readSymbolFiles, aggregatePerThreadArray, readTimingStruct
import numpy as np


def calcInvokedApis(scalerDataFolder, recInfo):
    invokedAPIs = []
    totalAPIs = []
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder, threadId)
        curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]
        invokedAPIs.append(len(curThreadInvokedRecArray))
        totalAPIs.append(len(curThreadRecArray))
    return invokedAPIs, totalAPIs


def printCoverageReport(scalerDataFolder, recInfo, invokedAPIs, totalAPIs):
    '''
    Coverage Report
    '''
    print(' \t ', end='')
    for threadId in recInfo.threadIdList:
        print(threadId, end='\t')
    print()

    print('Invoked %', end='\t')
    for perc in np.array(invokedAPIs) / np.array(totalAPIs):
        print('%2.2f%%' % (perc * 100), end='\t')
    print()

    print('Invoked APIs', end='\t')
    for invokedAPI in invokedAPIs:
        print(invokedAPI, end='\t')

    print()
    print('Total APIs', end='\t')
    for totalAPI in totalAPIs:
        print(totalAPI, end='\t')


def drawCountingHist(scalerDataFolder, recInfo):
    '''
    Counting histogram Report
    '''
    histogramRoot = os.path.join(scalerDataFolder, 'InvocationCountHist')
    if not os.path.exists(histogramRoot):
        os.mkdir(histogramRoot)

    print()

    # create 3 data sets with 1,000 samples
    mu, sigma = 200, 25
    x = mu + sigma * np.random.randn(1000, 3)

    totalCountArr = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        if totalCountArr is None:
            totalCountArr = times
        else:
            totalCountArr = np.vstack([totalCountArr, times])

        plt.figure()
        plt.hist(times, range=(1, times.max()), edgecolor="black", bins=50)
        plt.xlabel('Invocation counts')
        plt.ylabel('API number')
        plt.title('Histogram of invocation counts for thread %s' % (str(threadId)))

        plt.savefig(os.path.join(histogramRoot, threadId + '.png'))
        plt.close()
        # print(threadId, np.max(times))

    totalCountArr = totalCountArr.transpose()
    # totalCountArr = totalCountArr[np.where(totalCountArr > 0)]
    plt.figure()
    print(totalCountArr.max())
    plt.hist(totalCountArr, range=(1, totalCountArr.max()), bins=50, stacked=True)
    plt.xlabel('Invocation counts')
    plt.ylabel('API number')
    plt.title('Histogram of invocation counts for all threads staked')
    plt.savefig(os.path.join(histogramRoot, 'total.png'))
    plt.close()


def printInvocNumberPerThread(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return

    recInfo = readSymbolFiles(scalerDataFolder)
    invokedAPIs, totalAPIs = calcInvokedApis(scalerDataFolder, recInfo)

    totalInvocationCnts = 0

    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        totalInvocationCnts += np.sum(times)

    min = np.min(invokedAPIs)
    q1 = np.quantile(invokedAPIs, 0.25)
    q2 = np.quantile(invokedAPIs, 0.5)
    q3 = np.quantile(invokedAPIs, 0.75)
    q4 = np.quantile(invokedAPIs, 1)
    iqr = q3 - q1
    qLower = q1 - 1.5 * iqr
    qUpper = q3 + 1.5 * iqr
    assert (np.unique(totalAPIs).shape[0] == 1)
    assert (q4 == np.max(invokedAPIs))
    print(scalerDataFolder.split('/')[-2], min, q1, q2, q3, q4, iqr, qLower, qUpper, len(invokedAPIs), totalAPIs[0],
          totalInvocationCnts, sep='\t')


def printInvocCntPerAPI(scalerDataFolder):
    if scalerDataFolder is None:
        print()
        return

    recInfo = readSymbolFiles(scalerDataFolder)

    totalCountArr = None
    for threadId in recInfo.threadIdList:
        curThreadRecArray = readTimingStruct(scalerDataFolder, threadId)
        # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

        times = np.array([rec.count for rec in curThreadRecArray])
        if totalCountArr is None:
            totalCountArr = times
        else:
            totalCountArr = np.vstack([totalCountArr, times])
    totalCountArr = np.array(totalCountArr).sum(axis=0)

    totalCountArr = totalCountArr[np.where(totalCountArr > 0)]
    min = np.min(totalCountArr)
    q1 = np.quantile(totalCountArr, 0.25)
    q2 = np.quantile(totalCountArr, 0.5)
    q3 = np.quantile(totalCountArr, 0.75)
    q4 = np.quantile(totalCountArr, 1)
    iqr = q3 - q1
    qLower = q1 - 1.5 * iqr
    qUpper = q3 + 1.5 * iqr

    assert (q4 == np.max(totalCountArr))
    print(scalerDataFolder.split('/')[-2], min, q1, q2, q3, q4, iqr, qLower, qUpper, np.sum(totalCountArr), sep='\t')


# steven@masslabserv1:~/Downloads/2022-11-23_10-21-06$ find . -name "scalerdata*"
scalerDataFolders = [
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.blackscholes_0/scalerdata_19148850692747664',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.bodytrack_0/scalerdata_19148905483325260',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.canneal_0/scalerdata_19149009421840348',
    None,
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.facesim_0/scalerdata_19149183735878138',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.ferret_0/scalerdata_19149441937366104',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.fluidanimate_0/scalerdata_19149498481345624',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.freqmine_0/scalerdata_19149660473046832',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.raytrace_0/scalerdata_19149730167129240',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.streamcluster_0/scalerdata_19150235160442436',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.swaptions_0/scalerdata_19150507898053624',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.vips_0/scalerdata_19150561039693292',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.benchmarksuite.parsec.parsec3_0.x264_0/scalerdata_19150582352742288',
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.db.memcached.memcached_1_6_17_0/scalerdata_19150608805586386',
    None,
    '/media/umass/datasystem/steven/Downloads/2022-11-23_10-21-06/Application.db.redis.redis_7_0_4_0/scalerdata_19150762055485288',
    None,
    None,
    None,
    None,
    None,
]

print('Thread inovked API # imbalance Analysis')
for scalerDataFolder in scalerDataFolders:
    printInvocNumberPerThread(scalerDataFolder)

print('API inovked CNT Analysis')
for scalerDataFolder in scalerDataFolders:
    printInvocCntPerAPI(scalerDataFolder)
