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

from util.Analyzer.XFA import generateXFAStruct
import pandas as pd
import struct
from datastructure.TimingStruct import FileRecord, RecTuple
from matplotlib import pyplot as plt
import numpy as np
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/home/st/Downloads/ScalerOutput4EffectivenessApps 2023-11-17_18-33-51-066816-UTC-0500/Scaler/Application.ScalerEffectiveness.canneal.unfixed/0/ScalerOutput/scalerdata_26125423189876064'  # '/tmp/scalerdata_17245094620564'

recInfo = readSymbolFiles(scalerDataFolder)

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


# print(timingRecord)
# ,'libc-2.27.so','libm-2.27.so'
fileNameColorMap = {'libstdc++.so.6.0.25': FileDisplayParameter('libstdc++', '#E06666'),
                    'Self': FileDisplayParameter('Self', '#4285f4'),
                    'libpthread-2.27.so': FileDisplayParameter('libpthread', '#34a853'),
                    'libc-2.27.so': FileDisplayParameter('libc', '#ff6d01'),
                    'libm-2.27.so': FileDisplayParameter('libm', '#fbbc04'),
                    'Wait': FileDisplayParameter('Wait', '#46bdc6'),
                    'Join': FileDisplayParameter('Join', '#8142f4'),
                    'Others': FileDisplayParameter('Others', '#a5a5a5')
                    }


class FileNameClockCycleAndId:
    def __init__(self, fileName, clockCycle, fileId, color):
        self.displayName = fileName
        self.clockCycle = clockCycle
        self.fileId = fileId
        self.color = color


def getConciseFileName(absolutePath: str):
    # Get fileName
    fileName = absolutePath.split(os.sep)[-1]
    assert (fileName in fileNameColorMap)
    return fileNameColorMap[fileName].fileName


def getWedgeColor(absolutePath: str):
    fileName = absolutePath.split(os.sep)[-1]
    assert (fileName in fileNameColorMap)
    return fileNameColorMap[fileName].color


def drawXFAComponentView(callerId, calleeFileId):
    curTimingRecord = timingRecord[callerId]
    nameAndClockCycleList = []
    for fileId, curExtFileRecord in curTimingRecord.extFileTiming.items():
        nameAndClockCycleList.append(FileNameClockCycleAndId(getConciseFileName(curExtFileRecord.displayName),
                                                             curExtFileRecord.totalClockCycles.value, fileId,
                                                             getWedgeColor(curExtFileRecord.displayName)))
    nameAndClockCycleList.append(FileNameClockCycleAndId('Self', curTimingRecord.selfClockCycles.value, callerId,
                                                         getWedgeColor('Self')))
    nameAndClockCycleList.append(FileNameClockCycleAndId('Wait', curTimingRecord.waitClockCycles.value, -999,
                                                         getWedgeColor('Wait')))
    nameAndClockCycleList.append(FileNameClockCycleAndId('Join', curTimingRecord.joinClockCycles.value, -999,
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

    # Calculate pie-chart rotation angle
    calleePieChartWedgeIndex = None
    for i in range(len(nameAndClockCycleList)):
        if nameAndClockCycleList[i].fileId == calleeFileId:
            calleePieChartWedgeIndex = i

    startAngle = sum([v.clockCycle for v in nameAndClockCycleList[0:calleePieChartWedgeIndex]]) / clockCyclesSum * 360 + \
                 nameAndClockCycleList[calleePieChartWedgeIndex].clockCycle / 2 / clockCyclesSum * 360

    fig, ax = plt.subplots()
    ax.pie(np.array([v.clockCycle for v in nameAndClockCycleList]), counterclock=False,
           wedgeprops={'edgecolor': 'white', 'linewidth': 2},
           labels=[v.displayName for v in nameAndClockCycleList], colors=[v.color for v in nameAndClockCycleList],
           startangle=startAngle)

    selfFileName = curTimingRecord.fileName.split(os.sep)[-1]
    plt.text(0, 0, selfFileName, horizontalalignment='center', verticalalignment='center')
    plt.savefig("ComponentView_%s.svg" % (selfFileName))
    plt.show()
    print()


class APINameClockCycleAndId:
    def __init__(self, apiName, clockCycle, fileId, color):
        self.apiName = apiName
        self.clockCycle = clockCycle
        self.fileId = fileId
        self.color = color


def drawXFAAPIView(callerId, calleeFileId):

    callerFileName = timingRecord[callerId].fileName.split(os.sep)[-1]
    calleeFileName = timingRecord[calleeFileId].fileName.split(os.sep)[-1]

    curTimingRecord = timingRecord[callerId]
    nameAndClockCycleList = []
    for apiId, extSymRecord in curTimingRecord.extFileTiming[calleeFileId].extSymTiming.items():
        nameAndClockCycleList.append(APINameClockCycleAndId(extSymRecord.symbolName, extSymRecord.totalClockCycles.value, apiId, '#a5a5a5'))

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
    nameAndClockCycleList.append(APINameClockCycleAndId('Others', otherSum, -1, getWedgeColor('Others')))



    apiName = []
    apiValues = []

    # plt.text(0, 0, selfFileName, horizontalalignment='center', verticalalignment='center')
    fig, ax = plt.subplots()
    ax.pie(np.array([v.clockCycle for v in nameAndClockCycleList]), counterclock=False,
           wedgeprops={'edgecolor': 'white', 'linewidth': 2},
           labels=[v.apiName for v in nameAndClockCycleList], colors=[v.color for v in nameAndClockCycleList])
    plt.savefig("APIView_%s_%s.svg" % (callerFileName, calleeFileName))
    plt.show()
    print()


# Print for canneal
# for i in timingRecord[0].extFileTiming[6].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))
drawXFAComponentView(0, 8)
drawXFAAPIView(0, 8)
# Print for dedup malloc
# for i in timingRecord[0].extFileTiming[2].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))

# Print for swaptions hoard
# for i in timingRecord[7].extFileTiming[5].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))


# Print for dedup madvise
# for i in timingRecord[7].extFileTiming[1].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))


# for time in timingRecord:
#     print(time.fileName, time.selfClockCycles.value, sep='\t')

# print('FileName', 'Time', 'TimePercent(Self)', 'TimePercent(Global)', 'Count', 'CountPercent(Self)',
#       'CountPercent(Global)', sep='\t')
print('FileName', 'Time', 'Count', sep='\t')

for fileId, fileRecord in enumerate(timingRecord):
    # print(globalFileId, fileRecord.fileName, sep='\t')
    print(fileRecord.displayName.split(os.sep)[-1] + ' [S]', fileRecord.selfClockCycles.value, '-', sep='\t')
    for extFileId, extFileRecord in fileRecord.extFileTiming.items():
        print(extFileRecord.displayName.split(os.sep)[-1], extFileRecord.totalClockCycles.value,
              extFileRecord.counts.value, sep='\t')
    print('Wait', fileRecord.waitClockCycles.value, fileRecord.waitCount.value, sep='\t')
    print('Join', fileRecord.joinClockCycles.value, fileRecord.joinCount.value, sep='\t')
    print()

# API view canneal
# curTimingRecord=timingRecord[0]
# print('API view %s-%s'%(curTimingRecord.fileName.split(os.sep)[-1],curTimingRecord.extFileTiming[8].fileName.split(os.sep)[-1]))
# print('APIName', 'Time','Count', sep='\t')
# for curExtSymbolRecord in curTimingRecord.extFileTiming[8].extSymTiming.values():
#     # print(curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value,sep='\t')
#     os.system('echo "%s\\t%d\\t%d" | c++filt'%(curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value))

# API view dedup
# callerFileId=0
# calleeFileId=6
# curTimingRecord=timingRecord[callerFileId]
# print('API view %s-%s'%(curTimingRecord.fileName.split(os.sep)[-1],curTimingRecord.extFileTiming[calleeFileId].fileName.split(os.sep)[-1]))
# print('API Signature', 'Time','Count', sep='\t')
# for curExtSymbolRecord in curTimingRecord.extFileTiming[calleeFileId].extSymTiming.values():
#     # print(curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value,sep='\t')
#     os.system('echo "%s\\t%d\\t%d" | c++filt'%(curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value))

# API view dedup
callerFileId = 8
calleeFileId = 6
curTimingRecord = timingRecord[callerFileId]
print('API view %s-%s' % (
    curTimingRecord.displayName.split(os.sep)[-1],
    curTimingRecord.extFileTiming[calleeFileId].displayName.split(os.sep)[-1]))
print('API Signature', 'Time', 'Count', sep='\t')
for curExtSymbolRecord in curTimingRecord.extFileTiming[calleeFileId].extSymTiming.values():
    # print(curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value,sep='\t')
    os.system('echo "%s\\t%d\\t%d" | c++filt' % (
        curExtSymbolRecord.symbolName, curExtSymbolRecord.totalClockCycles.value, curExtSymbolRecord.counts.value))

# totalSelfTime = 0
# for fileRec in timingRecord:
#     if fileRec.selfClockCycles.value<0:
#         fileRec.selfClockCycles.value=0
#     totalSelfTime += fileRec.selfClockCycles.value
# for fileRec in timingRecord:
#     if fileRec.selfClockCycles.value<0:
#         fileRec.selfClockCycles.value=0
#     print(fileRec.fileName,'\t', round(fileRec.selfClockCycles.value / totalSelfTime * 100, 2))


#
# mysqlList= sorted(list(timingRecord[8].extFileTiming[7].extSymTiming.values()),key=lambda x: x.totalClockCycles.value,reverse=True)
#
# for item in mysqlList:
#     print('%s\t%s'%(item.symbolName,item.totalClockCycles.value))
# print('')

# Plot thread imbalance
print()
print('Thread\tName\tExecution Time - Wait Time')
for threadId, waitAndExecRecord in perThreadWaitAndExecutionTime.items():
    print('%s\t%s\t%d' % (
        threadId, waitAndExecRecord.threadName, waitAndExecRecord.totalExecutionTime - waitAndExecRecord.waitTime))

exit(0)
import matplotlib.pyplot as plt
import numpy as np

for curRecord in timingRecord:
    '''
    Component time view
    '''
    curFileName = curRecord.displayName.split(os.sep)[-1]
    fileNameList = ["Self", "Join", "Wait"]
    timeList = [curRecord.selfClockCycles.value, curRecord.joinClockCycles.value, curRecord.waitClockCycles.value]

    if curRecord.selfClockCycles.value == 0 and curRecord.childrenClockCycles.value == 0:
        continue

    for curFileId, curExtFileRecord in curRecord.extFileTiming.items():
        fileNameList.append(curExtFileRecord.displayName.split(os.sep)[-1])
        timeList.append(float(curExtFileRecord.totalClockCycles.value))

    '''
    API view
    '''
    for curFileId, curExtFileRecord in curRecord.extFileTiming.items():
        for curExtFileId, curExtFileRecord in curRecord.extFileTiming.items():
            apiNameList = []
            apiTimeList = []
            curApiFileName = curExtFileRecord.displayName.split(os.sep)[-1]

            for curAPIId, curExtSymTiming in curExtFileRecord.extSymTiming.items():
                apiNameList.append(curExtSymTiming.symbolName)
                apiTimeList.append(curExtSymTiming.totalClockCycles.value)

            fig, ax = plt.subplots()
            ax.pie(np.array(apiTimeList), labels=apiNameList)
            plt.title("APIView_time_%s_%s.svg" % (curFileName, curApiFileName))
            plt.savefig("APIView_time_%s_%s.svg" % (curFileName, curApiFileName))

    fig, ax = plt.subplots()
    ax.pie(np.array(timeList), labels=fileNameList)
    plt.title("ComponentView_time_%s.svg" % curFileName)
    plt.savefig("ComponentView_time_%s.svg" % curFileName)
