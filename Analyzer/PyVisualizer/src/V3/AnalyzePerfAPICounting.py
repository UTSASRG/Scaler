'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
import os
import traceback
from collections import defaultdict

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time


class StackElem:
    def __init__(self, libName, funcName):
        self.libName = libName
        self.funcName = funcName

    def __str__(self):
        return self.libName + " " + self.funcName


def analyzePerfScript(rootPath):
    libCallCountDict = defaultdict(int)
    apiCallCountDict = defaultdict(int)

    lastStack = []
    curStack = []
    with open(rootPath, 'r') as f:

        firstLine = True
        skipThisStak = False
        while True:

            if not firstLine:
                line = f.readline()
            else:
                line = '\n'
            if line == '\n':
                skipThisStak = False
                firstLine = False
                # print('New call stack')
                tmp = f.readline()

                if not tmp:
                    break

                if tmp.split()[0].strip() == 'perf':
                    skipThisStak = True
                # Calculate count based on call stack
                for i in range(len(lastStack)):
                    if i >= len(curStack) or lastStack[i].funcName != curStack[i].funcName:
                        for j in range(i, len(lastStack)):
                            libCallCountDict[lastStack[j].libName] += 1
                            apiCallCountDict[lastStack[j].funcName] += 1
                        break

                lastStack = curStack
                curStack = []
                if not tmp:
                    break
                continue
            else:
                if not skipThisStak:
                    line = line.strip().strip('\t').strip('\n')
                    addrEndI = line.find(' ')
                    addr = line[0:addrEndI]
                    libStartI = line.rfind(' ') + 1
                    libName = line[libStartI:]
                    funcNameAddr = line[addrEndI:libStartI].strip()
                    if funcNameAddr != '[unknown]' and libName != '([kernel.kallsyms])':
                        # print(libName)
                        plutInd = funcNameAddr.rfind('+')
                        funcName = funcNameAddr[0:plutInd]
                        addr = funcNameAddr[plutInd + 1:]
                        curStack.insert(0, StackElem(libName[1:-1], funcName))
    return libCallCountDict, apiCallCountDict


for ROOT_PATH in [
    '/home/steven/Projects/scalerbench/benchmarklog/PerfEffectivenessAfterThreadGroupChange 2023-11-28_22-42-26-307764-UTC-0500/Perf/Application.ScalerEffectiveness.dedup_I.unfixed/0/PerfOutput/perf.script'
]:
    _, apiCallCountDict = analyzePerfScript(ROOT_PATH)
    print(ROOT_PATH)
    itemList = list(apiCallCountDict.items())
    itemList = sorted(itemList, key=lambda x: x[0])
    for libName, counts in itemList:
        print(libName, counts, sep='\t')
