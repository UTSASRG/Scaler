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