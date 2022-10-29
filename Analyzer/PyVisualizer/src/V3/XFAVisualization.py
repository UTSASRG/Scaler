import math
import os
import pandas as pd
import struct
import re

from util.Analyzer.XFA import generateXFAStruct
from datastructure.TimingStruct import FileRecord, RecTuple
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/tmp/scalerdata_14568349363894428'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedStartingTime = aggregatePerThreadArray(scalerDataFolder, recInfo)
# Generate graph
timingRecord = generateXFAStruct(list(aggregatedTimeArray), aggregatedStartingTime, recInfo)

print(timingRecord)

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
