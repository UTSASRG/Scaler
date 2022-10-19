import os
import pandas as pd
import struct
import re
from datastructure.TimingStruct import FileRecord, RecTuple
from preProcessing import aggregatePerThreadArray, generateTimingStruct, calcPercentage, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/media/umass/datasystem/steven/scalerbench/mysql/issue48/workspace/scalerdata_12752056454442472'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedStartingTime = aggregatePerThreadArray(scalerDataFolder,recInfo)
# Generate graph
timingRecord = generateTimingStruct(list(aggregatedTimeArray), aggregatedStartingTime,recInfo)

print(timingRecord)

mysqlList= sorted(list(timingRecord[8].extFileTiming[7].extSymTiming.values()),key=lambda x: x.totalClockCycles.value,reverse=True)

for item in mysqlList:
    print('%s\t%s'%(item.symbolName,item.totalClockCycles.value))
print('')