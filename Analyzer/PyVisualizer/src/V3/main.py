import os
import pandas as pd
import struct
import re
from datastructure.TimingStruct import FileRecord, RecTuple
from preProcessing import aggregatePerThreadArray, generateTimingStruct, calcPercentage, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/media/umass/datasystem/steven/Downloads/scalerdata_8363066603580528'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedStartingTime = aggregatePerThreadArray(scalerDataFolder,recInfo)
# Generate graph
timingRecord = generateTimingStruct(list(aggregatedTimeArray), aggregatedStartingTime,recInfo)

print(timingRecord)