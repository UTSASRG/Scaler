import math
import os
import re

from util.Analyzer.XFA import generateXFAStruct
import pandas as pd
import struct
from datastructure.TimingStruct import FileRecord, RecTuple
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/tmp/scalerdata_1496479927241772'  # '/tmp/scalerdata_17245094620564'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedCreatorTime = aggregatePerThreadArray(scalerDataFolder, recInfo)
# The previous timing data is filtered, and thus it is necessary to record

# Generate graph
timingRecord = generateXFAStruct(list(aggregatedTimeArray), aggregatedCreatorTime, recInfo)

print(timingRecord)

# Print for canneal
# for i in timingRecord[0].extFileTiming[6].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))

# Print for dedup malloc
# for i in timingRecord[0].extFileTiming[2].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))

# Print for swaptions hoard
for i in timingRecord[7].extFileTiming[5].extSymTiming.values():
    os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))


# Print for dedup madvise
# for i in timingRecord[7].extFileTiming[1].extSymTiming.values():
#     os.system('echo "%s\\t%d" | c++filt'%(i.symbolName,i.totalClockCycles.value))


# for time in timingRecord:
#     print(time.fileName, time.selfClockCycles.value, sep='\t')

# print('FileName', 'Time', 'TimePercent(Self)', 'TimePercent(Global)', 'Count', 'CountPercent(Self)',
#       'CountPercent(Global)', sep='\t')
for fileId, fileRecord in enumerate(timingRecord):
    # print(fileId, fileRecord.fileName, sep='\t')
    print(fileRecord.fileName + ' [S]', fileRecord.selfClockCycles.value, fileRecord.selfClockCycles.localPercent,
          fileRecord.selfClockCycles.globalPercent, '-', '-', '-', sep='\t')
    for extFileId, extFileRecord in fileRecord.extFileTiming.items():
        print(extFileRecord.fileName, extFileRecord.totalClockCycles.value,
              extFileRecord.totalClockCycles.localPercent,
              extFileRecord.totalClockCycles.globalPercent, extFileRecord.counts.value,
              extFileRecord.counts.localPercent,
              extFileRecord.counts.globalPercent, sep='\t')
    print('Wait', fileRecord.waitClockCycles.value,
          fileRecord.waitClockCycles.localPercent,
          fileRecord.waitClockCycles.globalPercent, fileRecord.waitCount.value,
          fileRecord.waitCount.localPercent,
          fileRecord.waitCount.globalPercent, sep='\t')
    print('Join', fileRecord.joinClockCycles.value,
          fileRecord.joinClockCycles.localPercent,
          fileRecord.joinClockCycles.globalPercent, fileRecord.joinCount.value,
          fileRecord.joinCount.localPercent,
          fileRecord.joinCount.globalPercent, sep='\t')
    print()

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
