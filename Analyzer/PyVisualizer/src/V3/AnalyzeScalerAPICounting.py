'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
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

dataDir='/home/steven/Projects/scalerbench/benchmarklog/ScalerPerfArtifectforDedupAndFerretAfterThreadGroupAdjustment 2023-11-27_16-19-42-594494-UTC-0500'


def calcInvokedApiCNT(scalerDataFolder, recInfo):
    invokedAPICnts = []
    for threadId in recInfo.threadIdList:
        curThreadRecArray,_ = readTimingStruct(scalerDataFolder, threadId)
        invokedAPICnts.append(np.sum([rec.count for rec in curThreadRecArray]))
    return sum(invokedAPICnts)

resultDict = {}
for applicationName in os.listdir(os.path.join(dataDir, 'Scaler')):
    resultDict[applicationName] = list()
    for testTime in os.listdir(os.path.join(dataDir, 'Scaler', applicationName)):
        scalerDataFolder=os.path.join(dataDir, 'Scaler', applicationName, testTime, 'ScalerOutput')
        assert(len(os.listdir(scalerDataFolder))==1)
        scalerDataFolder=os.path.join(scalerDataFolder,os.listdir(scalerDataFolder)[0])

        recInfo = readSymbolFiles(scalerDataFolder)
        resultDict[applicationName].append(calcInvokedApiCNT(scalerDataFolder,recInfo))

for applicationName, countList in resultDict.items():
    print(applicationName, sum(countList) / len(countList),sep='\t')