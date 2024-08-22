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
from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

# scalerDataFolder = '/media/umass/datasystem/steven/benchmark/parsec/tests/dedup/scalerdata_30414326191467414'

scalerDataFolder = '/home/steven/Projects/scalerbench/effectiveness/ferret-unfixed/scalerdata_20139435311293788'  # '/tmp/scalerdata_17245094620564'

recInfo = readSymbolFiles(scalerDataFolder)

realFileId = None

aggregatedTimeArray, aggregatedCreatorTime = aggregatePerThreadArray(scalerDataFolder, recInfo)
# The previous timing data is filtered, and thus it is necessary to record

print('Thread creation time',aggregatedCreatorTime)
for symbolId, curRecordTuple in enumerate(aggregatedTimeArray):
    if curRecordTuple.totalClockCycles>0:
        print(symbolId,recInfo.symbolNameList[symbolId],
              'caller',recInfo.fileNameList[recInfo.symbolFileIdList[symbolId]].split(os.sep)[-1],
              'callee',recInfo.realFileIdList[symbolId], recInfo.fileNameList[recInfo.realFileIdList[symbolId]].split(os.sep)[-1],
              curRecordTuple.totalClockCycles)