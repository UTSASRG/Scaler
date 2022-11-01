from datastructure.TimingStruct import RecordingInfo, FileRecord


def generateXFAStruct(aggregatedTimeEntries, aggregatedStartingTime, recInfo: RecordingInfo):
    timingRecord = []  # Map file name to FileRecord struct
    mainFileId = None

    # Inset record to timingRecord
    for i in range(len(recInfo.fileNameList)):
        record = FileRecord()
        record.fileName = recInfo.fileNameList[i]
        timingRecord.append(record)

    # Attribute pthread_create to pthread library
    for i in range(len(recInfo.symbolNameList)):
        if recInfo.symbolNameList[i] == 'pthread_create':
            # Attribute this time to pthread library rather than scaler
            recInfo.realFileIdList[i] = recInfo.pthreadFileId

    totalProgramRunningTIme = 0
    totalInvocationCount = 0

    # Attribute the total thread running time
    for fileId, totalClockCycles in aggregatedStartingTime.items():
        timingRecord[fileId].selfClockCycles.value = totalClockCycles
        totalProgramRunningTIme += totalClockCycles

    # Loop through all timing entries and attribute time
    for i in range(len(aggregatedTimeEntries)):
        # Add current symbol time to corresponding file entry (Only record APIs that are invoked at least once)
        if aggregatedTimeEntries[i].count > 0:
            # Make sure symbol ID is correct
            if recInfo.realFileIdList[i] >= len(recInfo.fileNameList):
                print(
                    'Symbol %s was located in unhooked file. Is it overrided by an unhooked file?' %
                    recInfo.symbolNameList[
                        i])
                continue

            # Attribute time to callee
            curFileRecord = timingRecord[recInfo.symbolFileIdList[i]]
            curFileRecord.fileName = recInfo.fileNameList[recInfo.symbolFileIdList[i]]
            curFileRecord.selfClockCycles.value -= aggregatedTimeEntries[i].totalClockCycles
            curFileRecord.childrenClockCycles.value += aggregatedTimeEntries[i].totalClockCycles

            curExtFileRecord = curFileRecord.extFileTiming[recInfo.realFileIdList[i]]
            curExtFileRecord.fileName = recInfo.fileNameList[recInfo.realFileIdList[i]]
            curExtFileRecord.totalClockCycles.value += aggregatedTimeEntries[i].totalClockCycles
            curExtFileRecord.counts.value += aggregatedTimeEntries[i].count

            curExtSymRecord = curExtFileRecord.extSymTiming[recInfo.symIdInFileList[i]]
            curExtSymRecord.symbolName = recInfo.symbolNameList[i]
            curExtSymRecord.totalClockCycles.value = aggregatedTimeEntries[i].totalClockCycles
            curExtSymRecord.counts.value = aggregatedTimeEntries[i].count

            # Attribute time to caller
            realFileRecord = timingRecord[recInfo.realFileIdList[i]]
            # realFileRecord.fileName = fileNameList[recInfo.realFileIdList[i]]
            realFileRecord.selfClockCycles.value += aggregatedTimeEntries[i].totalClockCycles

            totalInvocationCount += aggregatedTimeEntries[i].count

    timingRecord = calcPercentage(timingRecord, totalProgramRunningTIme, totalInvocationCount)
    return timingRecord


def calcPercentage(timingRecord, programRuntime, totalApiCallCount):
    for curFileRecord in timingRecord:
        for curExtFileRecord in curFileRecord.extFileTiming.values():
            for curExtSymRecord in curExtFileRecord.extSymTiming.values():
                if curExtFileRecord.counts.value > 0:
                    curExtSymRecord.counts.parentPercent = curExtSymRecord.counts.value / curExtFileRecord.counts.value
                else:
                    curExtSymRecord.counts.parentPercent = 0.0
                if totalApiCallCount > 0:
                    curExtSymRecord.counts.globalPercent = curExtSymRecord.counts.value / totalApiCallCount
                else:
                    curExtSymRecord.counts.globalPercent = 0.0

                if curExtSymRecord.totalClockCycles.value > 0:
                    curExtSymRecord.totalClockCycles.parentPercent = curExtSymRecord.totalClockCycles.value / curExtFileRecord.totalClockCycles.value
                else:
                    curExtSymRecord.totalClockCycles.parentPercent = 0.0

                if programRuntime > 0:
                    curExtSymRecord.totalClockCycles.globalPercent = curExtSymRecord.totalClockCycles.value / programRuntime
                else:
                    curExtSymRecord.totalClockCycles.globalPercent = 0.0

            if totalApiCallCount > 0:
                curExtFileRecord.counts.globalPercent = curExtFileRecord.counts.value / totalApiCallCount
            else:
                curExtFileRecord.counts.globalPercent = 0.0

            if curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value > 0:
                curExtFileRecord.totalClockCycles.parentPercent = curExtFileRecord.totalClockCycles.value / (
                        curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
            else:
                curExtFileRecord.totalClockCycles.parentPercent = 0.0

            if programRuntime > 0:
                curExtFileRecord.totalClockCycles.globalPercent = curExtFileRecord.totalClockCycles.value / programRuntime
            else:
                curExtFileRecord.totalClockCycles.globalPercent = 0.0

        if curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value > 0:
            curFileRecord.selfClockCycles.totalPercent = curFileRecord.selfClockCycles.value / (
                    curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
        else:
            curFileRecord.selfClockCycles.totalPercent = 0.0

        if programRuntime > 0:
            curFileRecord.selfClockCycles.globalPercent = curFileRecord.selfClockCycles.value / programRuntime
        else:
            curFileRecord.selfClockCycles.globalPercent = 0.0

        if curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value > 0:
            curFileRecord.childrenClockCycles.totalPercent = curFileRecord.childrenClockCycles.value / (
                    curFileRecord.selfClockCycles.value + curFileRecord.childrenClockCycles.value)
        else:
            curFileRecord.childrenClockCycles.totalPercent = 0.0

        if programRuntime > 0:
            curFileRecord.childrenClockCycles.globalPercent = curFileRecord.childrenClockCycles.value / programRuntime
        else:
            curFileRecord.childrenClockCycles.globalPercent = 0.0

    return timingRecord
