import math
import os
import pandas as pd
import struct
import re

from util.Analyzer.XFA import generateXFAStruct
from datastructure.TimingStruct import FileRecord, RecTuple
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/tmp/tmp5_2108qs/libScalerHook-HookAutoAsm-C.so/scalerdata_454232674532264'
scalerDataFolder = '/tmp/scalerdata_457030350800482'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedCreatorTime = aggregatePerThreadArray(scalerDataFolder,  recInfo)
# The previous timing data is filtered, and thus it is necessary to record


# Generate graph
timingRecord = generateXFAStruct(list(aggregatedTimeArray),aggregatedCreatorTime, recInfo)

print(timingRecord)

for time in timingRecord:
    print(time.fileName, time.selfClockCycles.value, sep='\t')

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
print('')
