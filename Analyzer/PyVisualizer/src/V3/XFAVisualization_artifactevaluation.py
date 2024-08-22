'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
import math
import os
import re
import argparse
import pandas
from util.Analyzer.XFA import generateXFAStruct
import pandas as pd
import struct
from datastructure.TimingStruct import FileRecord, RecTuple
from matplotlib import pyplot as plt
from matplotlib.colors import to_rgb, rgb_to_hsv, hsv_to_rgb, to_hex
import numpy as np
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles, adjustThreadCreatorInfo
from enum import Enum
from pathlib import Path
from collections import defaultdict
import subprocess


parser = argparse.ArgumentParser()
parser.add_argument("scalerFolder",type=str,help='Scaler recording data folder')
parser.add_argument("outputFolder",type=str,help='Scaler output folder')


args = parser.parse_args()


timePieChartSaveFolder=os.path.join(args.outputFolder,'PieCharts','Time')
countPieChartSaveFolder=os.path.join(args.outputFolder,'PieCharts','Count')
threadImbalanceSaveFolder=os.path.join(args.outputFolder,'ThreadImbalance')
Path(timePieChartSaveFolder).mkdir(parents=True,exist_ok=True)
Path(countPieChartSaveFolder).mkdir(parents=True,exist_ok=True)
Path(threadImbalanceSaveFolder).mkdir(parents=True,exist_ok=True)


# class syntax

class GraphType(Enum):
    COUNT = 1
    TIME = 2



scalerDataFolder = args.scalerFolder

recInfo = readSymbolFiles(scalerDataFolder)
recInfo = adjustThreadCreatorInfo(recInfo)
adjustThreadCreatorInfo(recInfo)

'''
Save another explicit list of symbol file
'''
with open(os.path.join(args.outputFolder,'PieCharts','SymboleName.txt'),'w') as f:
    for symbolId, symbolName in enumerate(recInfo.symbolNameList):
        f.write('ID:%d Name:%s\n'%(symbolId,symbolName))

realFileId = None

aggregatedTimeArray, aggregatedCreatorTime, perThreadWaitAndExecutionTime = aggregatePerThreadArray(scalerDataFolder,
                                                                                                    recInfo, True)
# The previous timing data is filtered, and thus it is necessary to record

# Generate graph
timingRecord = generateXFAStruct(list(aggregatedTimeArray), aggregatedCreatorTime, recInfo)

print(aggregatedCreatorTime)


class FileDisplayParameter:
    def __init__(self, fileName, color):
        self.fileName = fileName
        self.color = color



class FileNameClockCycleAndId:
    def __init__(self, fileName, clockCycle, fileId, color):
        self.displayName = fileName
        self.clockCycle = clockCycle
        self.fileId = fileId
        self.color = color


def getConciseFileName(absolutePath: str):
    # Get fileName
    return absolutePath.split(os.sep)[-1]


def getWedgeColor(absolutePath: str):
    return absolutePath.split(os.sep)[-1]


def drawXFAComponentView(callerId, graphType):
    curTimingRecord = timingRecord[callerId]
    nameAndClockCycleList = []
    for fileId, curExtFileRecord in curTimingRecord.extFileTiming.items():
        if curExtFileRecord.counts.value > 0:
            if graphType == GraphType.TIME:
                nameAndClockCycleList.append(FileNameClockCycleAndId(getConciseFileName(curExtFileRecord.fileName),
                                                                     curExtFileRecord.totalClockCycles.value, fileId,
                                                                     getWedgeColor(curExtFileRecord.fileName)))
            else:
                nameAndClockCycleList.append(FileNameClockCycleAndId(getConciseFileName(curExtFileRecord.fileName),
                                                                     curExtFileRecord.counts.value, fileId,
                                                                     getWedgeColor(curExtFileRecord.fileName)))
    if(len(nameAndClockCycleList)==0):
        return
    
    if graphType == GraphType.TIME:
        nameAndClockCycleList.append(FileNameClockCycleAndId('Self', curTimingRecord.selfClockCycles.value, callerId,
                                                             getWedgeColor('Self')))
        nameAndClockCycleList.append(FileNameClockCycleAndId('Wait', curTimingRecord.waitClockCycles.value, -999,
                                                             getWedgeColor('Wait')))
        nameAndClockCycleList.append(FileNameClockCycleAndId('Join', curTimingRecord.joinClockCycles.value, -999,
                                                             getWedgeColor('Join')))
    else:
        nameAndClockCycleList.append(FileNameClockCycleAndId('Wait', curTimingRecord.waitCount.value, -999,
                                                             getWedgeColor('Wait')))
        nameAndClockCycleList.append(FileNameClockCycleAndId('Join', curTimingRecord.joinCount.value, -999,
                                                             getWedgeColor('Join')))

    clockCyclesSum = sum([v.clockCycle for v in nameAndClockCycleList])
    threshold = clockCyclesSum * 0.05
    sortedClockCycle = sorted(nameAndClockCycleList, key=lambda k: k.clockCycle)
    # Add the "other" wedge. Make sure the wedge that sums to < 5% of the total is merged into a single "Ohters" wedge.
    nameAndClockCycleList = []
    otherSum = 0
    for i in range(len(sortedClockCycle)):
        otherSum += sortedClockCycle[i].clockCycle
        if otherSum > threshold:
            nameAndClockCycleList.extend(sortedClockCycle[i:])
            otherSum -= sortedClockCycle[i].clockCycle
            break
    nameAndClockCycleList.append(FileNameClockCycleAndId('Others', otherSum, -1, getWedgeColor('Others')))


    fig, ax = plt.subplots()
    ax.pie(np.array([v.clockCycle for v in nameAndClockCycleList]), counterclock=False,
           wedgeprops={'edgecolor': 'white', 'linewidth': 2},
           labels=[v.displayName for v in nameAndClockCycleList],
           autopct='%1.0f%%')

    selfFileName = curTimingRecord.fileName.split(os.sep)[-1]
    # plt.text(0, 0, selfFileName, horizontalalignment='center', verticalalignment='center')
    if graphType == GraphType.TIME:
        plt.savefig(os.path.join(timePieChartSaveFolder,"ComponentView_%s.svg" % (selfFileName)))
    else:
        plt.savefig(os.path.join(countPieChartSaveFolder,"ComponentView_%s.svg" % (selfFileName)))
    plt.show()
    print()


def printThreadImbalance(perThreadWaitAndExecutionTime):
    print('Thread\tName\tExecution Time - Wait Time')
    threadGroupViewDict = defaultdict(int)
    singleThreadView=[]
    for threadId, waitAndExecRecord in perThreadWaitAndExecutionTime.items():
        threadGroupViewDict[waitAndExecRecord.threadName]+=waitAndExecRecord.totalExecutionTime - waitAndExecRecord.waitTime
    for threadId, waitAndExecRecord in perThreadWaitAndExecutionTime.items():
       singleThreadView.append(waitAndExecRecord.totalExecutionTime - waitAndExecRecord.waitTime)
    plt.figure()
    plt.ylabel('Time')
    plt.xlabel('Thread id')
    plt.bar(list(range(len(singleThreadView))),singleThreadView)
    plt.savefig(os.path.join(threadImbalanceSaveFolder, "singleThreads.svg"))

    plt.figure()
    plt.xticks(rotation=30)
    plt.ylabel('Time')
    plt.xlabel('Thread Name')
    plt.bar(threadGroupViewDict.keys(),threadGroupViewDict.values())
    plt.savefig(os.path.join(threadImbalanceSaveFolder, "ThreadImbalance_aggregatedByThreadName.svg"))

class APINameClockCycleAndId:
    def __init__(self, apiName, clockCycle, symbolId, color):
        self.apiName = apiName
        self.clockCycle = clockCycle
        self.symbolId = symbolId
        self.color = color


def drawXFAAPIView(callerId, calleeFileId, graphType):
    callerFileName = timingRecord[callerId].fileName.split(os.sep)[-1]
    calleeFileName = timingRecord[calleeFileId].fileName.split(os.sep)[-1]

    curTimingRecord = timingRecord[callerId]

    nameAndClockCycleList = []
    for apiId, extSymRecord in curTimingRecord.extFileTiming[calleeFileId].extSymTiming.items():
        if graphType == GraphType.TIME:
            nameAndClockCycleList.append(
                APINameClockCycleAndId(extSymRecord.symbolName, extSymRecord.totalClockCycles.value, apiId, None))
        else:
            nameAndClockCycleList.append(
                APINameClockCycleAndId(extSymRecord.symbolName, extSymRecord.counts.value, apiId, None))
    if len(nameAndClockCycleList) ==0:
        return
    clockCyclesSum = sum([v.clockCycle for v in nameAndClockCycleList])
    threshold = clockCyclesSum * 0.1
    sortedClockCycle = sorted(nameAndClockCycleList, key=lambda k: k.clockCycle)

    # Add the "other" wedge. Make sure the wedge that sums to < 5% of the total is merged into a single "Ohters" wedge.
    # Get callee's color

    nameAndClockCycleList = []

    otherSum = 0
    for i in range(len(sortedClockCycle)):
        otherSum += sortedClockCycle[i].clockCycle
        if otherSum > threshold:


            for j in range(i, len(sortedClockCycle)):
                sortedClockCycle[j].apiName = sortedClockCycle[j].apiName  # apiNameMap[sortedClockCycle[j].apiName]
                nameAndClockCycleList.append(sortedClockCycle[j])
                print(sortedClockCycle[j].apiName)

            otherSum -= sortedClockCycle[i].clockCycle
            break
    nameAndClockCycleList.append(APINameClockCycleAndId('Others', otherSum, -1, getWedgeColor('Others')))

    apiName = []
    apiValues = []

    # plt.text(0, 0, selfFileName, horizontalalignment='center', verticalalignment='center')
    fig, ax = plt.subplots()
    ax.pie(np.array([v.clockCycle for v in nameAndClockCycleList]), counterclock=False,
           wedgeprops={'edgecolor': 'white', 'linewidth': 2},
           labels=[str(v.symbolId)+':'+v.apiName for v in nameAndClockCycleList],
           autopct='%1.0f%%'
           )
    if graphType == GraphType.TIME:
        plt.savefig(os.path.join(timePieChartSaveFolder,"APIView_%s_%s.svg" % (callerFileName, calleeFileName)))
    else:
        plt.savefig(os.path.join(countPieChartSaveFolder,"APIView_%s_%s.svg" % (callerFileName, calleeFileName)))
    plt.show()
    print()



printThreadImbalance(perThreadWaitAndExecutionTime)

import matplotlib.pyplot as plt
import numpy as np


for curFileId,curRecord in enumerate(timingRecord):
    '''
    Component time view
    '''
    curFileName = curRecord.fileName.split(os.sep)[-1]
    fileNameList = ["Self", "Join", "Wait"]
    timeList = [curRecord.selfClockCycles.value, curRecord.joinClockCycles.value, curRecord.waitClockCycles.value]

    if curRecord.selfClockCycles.value == 0 and curRecord.childrenClockCycles.value == 0:
        continue

    drawXFAComponentView(curFileId,GraphType.TIME)
    drawXFAComponentView(curFileId,GraphType.COUNT)

    '''
    API view
    '''
    for curExtFileId, curExtFileRecord in curRecord.extFileTiming.items():
        drawXFAAPIView(curFileId,curExtFileId,GraphType.TIME)
        drawXFAAPIView(curFileId,curExtFileId,GraphType.COUNT)
