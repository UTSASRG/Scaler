import os
import shutil
import traceback

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time

from Analyzer.PyVisualizer.src.V3.datastructure.TimingStruct import ArrayDescriptor
from Analyzer.PyVisualizer.src.V3.util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from Analyzer.PyVisualizer.src.V3.util.Parser.TimeOutputPrarser import readSymbolFiles


def shouldPlt(timingArr):
    return timingArr.shape[0] > 500


def saveSinglePlot(ROOT_PATH, symbolNameList, threadId, tgtSymId, graphType='hist'):
    # hasPoints = False
    # ROOT_PATH: str, symbolNameList: list, threadId: str, tgtSymIds: list):
    detailedTimingDict = parseSingleSymDetailedTiming(ROOT_PATH, threadId, [tgtSymId])

    for symId, detailedTimingArr in detailedTimingDict.items():
        fig, axes = plt.subplots(nrows=1,ncols=2)
        ax1, ax2 = axes

        if not shouldPlt(detailedTimingArr):
            continue

        if graphType == 'hist':
            ax1.hist(detailedTimingArr, range=(1, detailedTimingArr.max()), edgecolor="black", bins=50)
            # print(os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
            # if hasPoints:
        elif graphType == 'scatter':
            ax1.scatter(np.arange(detailedTimingArr.shape[0]), detailedTimingArr, s=10)
        else:
            assert (False)

        if not os.path.exists(os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId])):
            os.makedirs(os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId]), exist_ok=True)
        fig.savefig(
            os.path.join(ROOT_PATH, 'DetailedTime', graphType, symbolNameList[symId],
                         'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
        # print(os.path.join(ROOT_PATH, 'threadDetailedTiming_%d_%s_%s.png' % (symId, symbolNameList[symId], threadId)))
        plt.close(fig)
    return 0


def error_callback(e):
    print('error')
    print(dir(e), "\n")
    print("-->{}<--".format(e.__cause__))


def doIt(ROOT_PATH, pool, rltList):
    print('========================', ROOT_PATH)
    allFiles = os.listdir(ROOT_PATH)
    symbolNum = 0

    recInfo = readSymbolFiles(ROOT_PATH)
    threadSymInfo = dict({})  # Threadid : symbol size
    for fileName in allFiles:
        if fileName.startswith('threadDetailedTiming') and fileName.endswith('.bin'):
            _, threadId = fileName.replace('.bin', '').split('_')
            with open(os.path.join(ROOT_PATH, fileName), 'rb') as f:
                symDetailedTimingDesc = ArrayDescriptor()
                f.readinto(symDetailedTimingDesc)
                assert (symDetailedTimingDesc.arrayElemSize == 0)
                assert (symDetailedTimingDesc._magicNum == 167)
                symbolNum = symDetailedTimingDesc.arraySize
                threadSymInfo[threadId] = symbolNum

            for symId in range(symbolNum):
                res = pool.apply_async(saveSinglePlot,
                                       args=[ROOT_PATH, recInfo.symbolNameList, threadId, symId, 'scatter'],
                                       error_callback=error_callback)
                rltList.append(res)

    return rltList


pool = Pool(64)
rltList = []
scalerDataFolders = [
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.blackscholes_0/Scaler-DETAIL-Artifects/scalerdata_1098304170115468',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.bodytrack_0/Scaler-DETAIL-Artifects/scalerdata_1098358945736648',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.canneal_0/Scaler-DETAIL-Artifects/scalerdata_1098456760642602',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.dedup_0/Scaler-DETAIL-Artifects/scalerdata_1098609218045480',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.facesim_0/Scaler-DETAIL-Artifects/scalerdata_1098633517549600',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.ferret_0/Scaler-DETAIL-Artifects/scalerdata_1098896081465298',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.fluidanimate_0/Scaler-DETAIL-Artifects/scalerdata_1107220283374448',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.freqmine_0/Scaler-DETAIL-Artifects/scalerdata_1099112583217984',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.raytrace_0/Scaler-DETAIL-Artifects/scalerdata_1099182362951380',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.streamcluster_0/Scaler-DETAIL-Artifects/scalerdata_1099698097420154',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.swaptions_0/Scaler-DETAIL-Artifects/scalerdata_1099956187630596',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.vips_0/Scaler-DETAIL-Artifects/scalerdata_1100002948879490',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.benchmarksuite.parsec.parsec3_0.x264_0/Scaler-DETAIL-Artifects/scalerdata_1100026471754668',
    # '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.webserver.httpd.httpd_2_4_54_0/Scaler-DETAIL-Artifects/scalerdata_1100053088973938',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.memcached.memcached_1_6_17_0/Scaler-DETAIL-Artifects/scalerdata_1100215829624386',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.mysql.mysql_8_0_31_0/Scaler-DETAIL-Artifects/scalerdata_1100366950684482',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.webserver.nginx.nginx_1_23_2_0/Scaler-DETAIL-Artifects/scalerdata_1100701228337962',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.redis.redis_7_0_4_0/Scaler-DETAIL-Artifects/scalerdata_1100850174945384',
    '/media/umass/datasystem/steven/Downloads/performancetest20221124/2022-11-30_19-51-43-DetailedTiming/Application.db.sqlite.sqlite_3_39_4_0/Scaler-DETAIL-Artifects/scalerdata_1101150204449494'
]

for ROOT_PATH in scalerDataFolders:
    if ROOT_PATH is not None:
        recInfo = readSymbolFiles(ROOT_PATH)
        doIt(ROOT_PATH, pool, rltList)

pool.close()
while len(rltList) > 0:
    time.sleep(2)
    rltList = [rlt for rlt in rltList if not rlt.ready()]
    print("%d jobs left" % len(rltList))
pool.join()
