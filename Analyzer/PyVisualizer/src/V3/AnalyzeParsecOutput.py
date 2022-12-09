import os
import numpy as np
import tempfile
import subprocess

'''
This script parses the output of parsecmgmt
'''


def findAppStart(lines):
    benchmarkName = []
    outputTuple = []
    for id, line in enumerate(lines):
        # Find start
        if line.startswith('[PARSEC] [========== Running '):
            benchmarkName.append(line.replace('[PARSEC] [========== Running benchmark', '')
                                 .replace(' [1] ==========]', '').strip())
            outputTuple.append([id, 0])

    curI = 0
    for id, line in enumerate(lines):
        # Find start
        if line.startswith('[PARSEC] [----------    End of output    ----------]'):
            outputTuple[curI][1] = id
            curI += 1
    return benchmarkName, outputTuple


def extractRealTimeAndMemory(lines, outputTuple):
    realTimeList = []
    memList = []
    for startI, endI in outputTuple:
        lineSplit = lines[endI - 1].split(',')
        realTIme = lineSplit[0].split(':')[1].strip()
        memUsg = lineSplit[-1].split(':')[1].strip()

        realTimeList.append(float(realTIme))
        memList.append(float(memUsg))
    return np.array(realTimeList), np.array(memList)


def parseOutputFile(PARSEC_OUTPUT_FOLDER):
    timeArray = None
    memArray = None
    benchmarkName = None

    for textFileName in os.listdir(PARSEC_OUTPUT_FOLDER):
        with open(os.path.join(PARSEC_OUTPUT_FOLDER, textFileName)) as f:
            assert (str.isdigit(textFileName[:-4]))  # Make sure the first id is
            lines = f.readlines()
            benchmarkName, outputTuple = findAppStart(lines)
            realTimeList, memList = extractRealTimeAndMemory(lines, outputTuple)
            if timeArray is None:
                timeArray = realTimeList
            if memArray is None:
                memArray = memList
            timeArray = np.vstack([timeArray, realTimeList])
            memArray = np.vstack([memArray, memList])

    return benchmarkName, memArray, timeArray


def printMatrix(matrixName, preloadList, benchmarkNameList, matrix):
    print(matrixName)
    print('       \t', end='')
    for benchmarkName in benchmarkNameList:
        print(benchmarkName, end='\t')
    print()

    for row in range(matrix.shape[0]):
        print(preloadList[row][0], end='\t')
        for col in range(matrix.shape[1]):
            print(matrix[row][col], end='\t')
        print()
    print()


def runBenchmark(runTimes, configNames, preloadList, inputType, threadNum, outputFolder):
    for (preloadName, preloadCmd) in preloadList:
        curOutputDir = os.path.join(outputFolder, preloadName)
        os.mkdir(curOutputDir)
        for curRunId in range(runTimes):
            curOutputFile = os.path.join(curOutputDir, '%d.txt' % curRunId)

            cmd = 'parsecmgmt -a run'
            cmd += ''.join([''.join([' -p ', config]) for config in configNames])
            if preloadCmd.strip() != '':
                cmd += ''.join([' -s "', preloadCmd, '"'])
            cmd += ''.join([' -n ', str(threadNum)])
            cmd += ''.join([' -i ', inputType])
            cmd += ''.join([' > ', curOutputFile])
            cmd += ''.join([' 2> ', curOutputFile])

            print('Running:', cmd)
            if os.system(cmd) != 0:
                print(cmd, 'failed')


def packOutput(preloadList, outputFolder):
    stdRealTimes = None
    avgRealTimes = None
    avgMem = None
    stdMem = None
    outputBenchNameList = None
    for (preloadName, preloadCmd) in preloadList:
        curOutputDir = os.path.join(outputFolder, preloadName)
        '''
        Calculate mean and average of the results
        '''
        outputBenchNameList, memArray, timeArray = parseOutputFile(curOutputDir)
        if avgRealTimes is None:
            avgRealTimes = np.average(timeArray, axis=0)
        else:
            avgRealTimes = np.vstack([avgRealTimes, np.average(timeArray, axis=0)])

        if stdRealTimes is None:
            stdRealTimes = np.std(timeArray, axis=0)
        else:
            stdRealTimes = np.vstack([stdRealTimes, np.std(timeArray, axis=0)])

        if avgMem is None:
            avgMem = np.average(memArray, axis=0)
        else:
            avgMem = np.vstack([avgMem, np.average(memArray, axis=0)])

        if stdMem is None:
            stdMem = np.std(memArray, axis=0)
        else:
            stdMem = np.vstack([stdMem, np.std(memArray, axis=0)])
    return avgRealTimes, stdRealTimes, avgMem, stdMem, outputBenchNameList


'''
Find max output id
'''

TIME_COMMAND = "/usr/bin/time -f 'real:%e, user:%U, sys:%S, memKB:%M' "
RUNTIMES = 1
CONFIG_NAMES = ['blackscholes', 'bodytrack', 'facesim', 'ferret', 'fluidanimate', 'freqmine', 'raytrace', 'swaptions',
                'vips', 'x264']
# PRELOAD_LIST = [('Default', TIME_COMMAND), ('Perf', TIME_COMMAND + 'perf record -g -o perf.data')]
PRELOAD_LIST = [('Scaler', 'export LD_PRELOAD=/media/umass/datasystem/steven/Scaler/cmake-build-release/libHook-c/libScalerHook-HookAutoAsm-C.so')]

outputFolder = tempfile.mkdtemp()
runBenchmark(RUNTIMES, CONFIG_NAMES, PRELOAD_LIST, 'simsmall', 64, outputFolder)

avgRealTimes, stdRealTimes, avgMem, stdMem, outputBenchNameList = packOutput(PRELOAD_LIST, outputFolder)
printMatrix('Table of average runtime', PRELOAD_LIST, outputBenchNameList, avgRealTimes)
printMatrix('Table of std runtime', PRELOAD_LIST, outputBenchNameList, stdRealTimes)
printMatrix('Table of average memory', PRELOAD_LIST, outputBenchNameList, avgMem)
printMatrix('Table of std memory', PRELOAD_LIST, outputBenchNameList, stdMem)
