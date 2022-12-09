import math
import os
import traceback
from collections import defaultdict

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time
import math

from Analyzer.PyVisualizer.src.V3.datastructure.TimingStruct import ArrayDescriptor
from Analyzer.PyVisualizer.src.V3.util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from Analyzer.PyVisualizer.src.V3.util.Parser.TimeOutputPrarser import readSymbolFiles, readTimingStruct
from Analyzer.PyVisualizer.src.V3.util.Quantile import calcQuantile


class APIMetric:
    def __init__(self):
        self.estimated = False
        self.estimatedSum = 0
        self.realSum = 0
        self.invocationCnt = 0

    def getErrorRate(self):
        assert (self.realSum > 0)
        return abs(self.realSum - self.estimatedSum) / self.realSum


def methodBoundAndNaiveClipping(symId, recInfo, timingArr):
    def shouldSkip(timingArr):
        if timingArr.shape[0] <= 500:
            return False
        mean = np.average(timingArr[0:500])
        meanUpperbound = mean * (1 + 0.50)
        meanLowerbound = mean * (1 - 0.50)

        c1 = np.where(timingArr[500:1000] > meanUpperbound)[0].shape[0]
        c2 = np.where(timingArr[500:1000] < meanLowerbound)[0].shape[0]
        return c1 == 0 and c2 == 0

    def removeOutliersByPercentage(x, lowerPerc, upperPerc):
        a = np.array(x)
        upper_quartile = np.percentile(a, upperPerc)
        lower_quartile = np.percentile(a, lowerPerc)
        return x[np.logical_and((lower_quartile <= x), (x <= upper_quartile))]

    skipped = shouldSkip(timingArr)
    estimatedSum = np.sum(timingArr)

    if skipped:
        outlierRemovedTimArray = removeOutliersByPercentage(timingArr[500:1000], 5, 95)
        estimatedSum = np.average(outlierRemovedTimArray) * timingArr.shape[0]

    return skipped, estimatedSum


def methodPreEstimation(symId, recInfo, timingArr):
    shouldSkip = False

    if timingArr.shape[0] <= 500:
        shouldSkip = False
    else:
        shouldSkip = True

    def removeOutliers(timingArr):
        rlt = []
        prevVal = timingArr[0]
        threshold = 100
        for i in range(1, timingArr.shape[0]):
            if abs(timingArr[i] - prevVal) < threshold:
                rlt.append(timingArr[i])
            else:
                rlt.append(prevVal)
            prevVal = timingArr[i]
        return np.array(rlt)

    realSum = np.sum(timingArr)

    if not shouldSkip:
        return shouldSkip, realSum

    # Test Prediction
    outlierRemovedFirst500 = removeOutliers(timingArr[0:500])
    estimatedSum = np.mean(outlierRemovedFirst500) * timingArr.shape[0]

    if abs(estimatedSum-realSum)/realSum < 0.02:
        shouldSkip=True
        return shouldSkip, estimatedSum
    else:
        shouldSkip=False
        return shouldSkip, realSum

def methodEstimation(symId, recInfo, timingArr):
    pass


def analyzeOutlierRemovalTechnique(ROOT_PATH, methodFunction):
    if ROOT_PATH is None:
        print()
        return
    allFiles = os.listdir(ROOT_PATH)
    symbolNum = 0

    allInvocationRelationCnt = 0
    skippedApiCnt = 0
    allNonZeroApiCnt = 0

    recInfo = readSymbolFiles(ROOT_PATH)
    threadSymInfo = dict({})  # Threadid : symbol size

    # print('=====> ', ROOT_PATH)
    apiMetricsPerApp = []
    totalAPICount = 0

    totalInvocationCnt = 0
    for fileName in allFiles:
        if fileName.startswith('threadDetailedTiming') and fileName.endswith('.bin'):
            # Read symbol number in threads
            _, threadId = fileName.replace('.bin', '').split('_')

            detailedTimingArr = parseSingleSymDetailedTiming(ROOT_PATH, threadId, None)
            recArrForThisThread = readTimingStruct(ROOT_PATH, threadId)
            totalAPICount = len(detailedTimingArr)
            apiMetricsPerThread = []

            for symId, timingArr in detailedTimingArr.items():
                curMetric = APIMetric()
                curMetric.estimated, curMetric.estimatedSum = methodFunction(symId, recInfo, timingArr)
                curMetric.realSum = np.sum(timingArr)
                curMetric.invocationCnt = recArrForThisThread[symId].count
                totalInvocationCnt += recArrForThisThread[symId].count
                if curMetric.realSum > 0 and curMetric.getErrorRate() > 0:
                    apiMetricsPerThread.append(curMetric)
                    apiMetricsPerApp.append(curMetric)
            # if len(apiMetricsPerThread) > 0:
            #     # Print table for estimated value per API
            #     minimum, q1, q2, q3, q4, iqr, qLower, qUpper = calcQuantile(
            #         [apiMetric.getErrorRate() for apiMetric in apiMetricsPerThread])
            #     estimatedNum = np.sum([1 for apiMetric in apiMetricsPerThread if apiMetric.estimated])
            #     totalAPICount = len(detailedTimingArr)
            #     print(fileName, minimum, q1, q2, q3, q4, iqr, qLower, qUpper, estimatedNum, totalAPICount, sep='\t')

    if len(apiMetricsPerApp) > 0:
        minimum, q1, q2, q3, q4, iqr, qLower, qUpper = calcQuantile(
            [apiMetric.getErrorRate() for apiMetric in apiMetricsPerApp])
        estimatedNum = np.sum([1 for apiMetric in apiMetricsPerApp if apiMetric.estimated])
        skippedApiCnt = np.sum([apiMetric.invocationCnt for apiMetric in apiMetricsPerApp if apiMetric.estimated])
        print(ROOT_PATH.split('/')[-3], minimum, q1, q2, q3, q4, iqr, qLower, qUpper, estimatedNum, totalAPICount,
              skippedApiCnt, skippedApiCnt / totalInvocationCnt, totalInvocationCnt,
              sep='\t')
    else:
        print(ROOT_PATH.split('/')[-2])


for ROOT_PATH in [
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
]:
    analyzeOutlierRemovalTechnique(ROOT_PATH, methodPreEstimation)
