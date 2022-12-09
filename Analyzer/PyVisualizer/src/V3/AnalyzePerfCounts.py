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
    callCountDict = defaultdict(int)

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
                            callCountDict[lastStack[j].libName] += 1
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
    return callCountDict


for ROOT_PATH in [
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.blackscholes_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.bodytrack_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.canneal_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.dedup_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.facesim_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.ferret_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.fluidanimate_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.freqmine_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.raytrace_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.streamcluster_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.swaptions_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.vips_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.benchmarksuite.parsec.parsec3_0.x264_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.db.memcached.memcached_1_6_17_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.db.mysql.mysql_8_0_31_0/Perf-Artifects/script.txt',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-27_19-31-45-Perf-4000/Application.db.redis.redis_7_0_4_0/Perf-Artifects/script.txt',
    '/media/umass/datasystem/steven/Downloads/Perf-Sampling-At4000/Application.db.sqlite.sqlite_3_39_4_0/Perf-Artifects/script.txt',
    '/media/umass/datasystem/steven/Downloads/Perf-Sampling-At4000/Application.webserver.httpd.httpd_2_4_54_0/Perf-Artifects/script.txt',
    '/media/umass/datasystem/steven/Downloads/Perf-Sampling-At4000/Application.webserver.nginx.nginx_1_23_2_0/Perf-Artifects/script.txt',
    # None,
    # None,
    # None,
    # None,
]:
    callDict = analyzePerfScript(ROOT_PATH)
    print(ROOT_PATH)
    itemList = list(callDict.items())
    itemList = sorted(itemList, key=lambda x: x[0])
    for libName, counts in itemList:
        print(libName, counts, sep='\t')
