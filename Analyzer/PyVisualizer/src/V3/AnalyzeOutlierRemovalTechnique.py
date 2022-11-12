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
from Analyzer.PyVisualizer.src.V3.util.Parser.TimeOutputPrarser import readSymbolFiles, readTimingStruct


def removeOutliersPercentage(x, lowerPerc, upperPerc):
    a = np.array(x)
    upper_quartile = np.percentile(a, upperPerc)
    lower_quartile = np.percentile(a, lowerPerc)
    return x[np.logical_and((lower_quartile <= x), (x <= upper_quartile))]


def shouldSkip(timingArr):
    if timingArr.shape[0] < 500:
        return False
    mean = np.average(timingArr[0:500])
    meanUpperbound = mean * (1 + 0.5)
    meanLowerbound = mean * (1 - 0.5)

    c1 = np.where(timingArr[500:1000] > meanUpperbound)[0].shape[0]
    c2 = np.where(timingArr[500:1000] < meanLowerbound)[0].shape[0]
    return c1 == 0 and c2 == 0


def analyzeApi(symId, recInfo, timingArr):
    # hasPoints = False
    # ROOT_PATH: str, symbolNameList: list, threadId: str, tgtSymIds: list):

    # skippedInvocationCnt = 0
    # allInvocationRelationCnt = 0
    # skippedApiCnt = 0
    # allNonZeroApiCnt = 0

    # allNonZeroApiCnt += 1
    # allInvocationRelationCnt += recStruct[symId].count

    skipped = shouldSkip(timingArr)
    errorScore = 0
    if skipped:
        print(recInfo.symbolNameList[symId], 'skipped')
        removedImg = removeOutliersPercentage(timingArr[500:1000], 5, 95)
        estimatedSum = np.average(removedImg) * 500
        trueSum = np.sum(timingArr[500:1000])
        errorScore = abs(estimatedSum - trueSum) / trueSum * 100
        print('Print Errors', estimatedSum, trueSum, abs(estimatedSum - trueSum) / trueSum * 100)

    # if allNonZeroApiCnt > 0:
    #     print('Skipped API Percentage', skippedApiCnt, allNonZeroApiCnt, skippedApiCnt / allNonZeroApiCnt * 100)
    # if skippedInvocationCnt > 0:
    #     print('Skipped Invocation Percentage', allInvocationRelationCnt, skippedInvocationCnt,
    #           allInvocationRelationCnt / skippedInvocationCnt * 100)

    return skipped, errorScore


def analyzeOutlierRemovalTechnique(ROOT_PATH):
    print('========================', ROOT_PATH)
    allFiles = os.listdir(ROOT_PATH)
    symbolNum = 0

    skippedInvocationCnt = 0
    allInvocationRelationCnt = 0
    skippedApiCnt = 0
    allNonZeroApiCnt = 0

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

            detailedTimingArr = parseSingleSymDetailedTiming(ROOT_PATH, threadId, None)
            recArrForThisThread = readTimingStruct(ROOT_PATH, threadId)

            for symId, timingArr in detailedTimingArr.items():
                skipped, errorRate = analyzeApi(symId, recInfo, timingArr)

                if recArrForThisThread[symId].count > 0:
                    # Only count non-zero api
                    if skipped:
                        skippedApiCnt += 1
                        skippedInvocationCnt += recArrForThisThread[symId].count
                    allNonZeroApiCnt += 1
                    allInvocationRelationCnt += recArrForThisThread[symId].count

    print('APIs removed percentage', skippedApiCnt / allNonZeroApiCnt * 100)
    print('Invocation Count Removed Percentage', skippedInvocationCnt / allInvocationRelationCnt * 100)


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
    print(ROOT_PATH)
    analyzeOutlierRemovalTechnique(ROOT_PATH)
