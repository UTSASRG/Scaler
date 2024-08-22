'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
import os
import re

scalerDataPath = ''

from util.Parser.TimeOutputPrarser import aggregatePerThreadArray, readSymbolFiles

scalerDataFolder = '/home/steven/Downloads/stdpthreadebug'

recInfo = readSymbolFiles(scalerDataFolder)

scalerOutput = ''
with open('/home/steven/Downloads/scalerdbg.txt', 'r') as f:
    scalerOutput = f.read()

callerFileIdRe = re.compile('CallerFileId:\d*')
funcIdRe = re.compile('Func:\d*')

# Replace callerfileId
curPos = 0
while True:
    searchRlt = callerFileIdRe.search(scalerOutput, curPos)
    if searchRlt is None:
        break
    startPos, endPos = searchRlt.span()
    curPos = endPos

    idStr = scalerOutput[startPos:endPos].split(':')
    simplifiedLibName = recInfo.fileNameList[int(idStr[1])].split(os.path.sep)[-1]
    idStr = ''.join([idStr[0], ':', simplifiedLibName])
    scalerOutput = ''.join([scalerOutput[0:startPos], idStr, scalerOutput[endPos:]])

# Replace funcId
curPos = 0
while True:
    searchRlt = funcIdRe.search(scalerOutput, curPos)
    if searchRlt is None:
        break
    startPos, endPos = searchRlt.span()
    curPos = endPos

    idStr = scalerOutput[startPos:endPos].split(':')
    idStr = ''.join([idStr[0], ':', recInfo.symbolNameList[int(idStr[1])]])
    scalerOutput = ''.join([scalerOutput[0:startPos], idStr, scalerOutput[endPos:]])

with open('/home/steven/Downloads/scalerdbg.txt', 'w') as f:
    f.write(scalerOutput)