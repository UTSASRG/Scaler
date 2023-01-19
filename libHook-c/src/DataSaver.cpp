#include <util/hook/DataSaver.h>
#include <util/tool/FileTool.h>
#include <util/hook/LogicalClock.h>
#include <util/tool/SpinLock.h>


#ifdef INSTR_TIMING
const int TIMING_REC_COUNT = 20000;
typedef int64_t TIMING_TYPE;
__thread TIMING_TYPE **detailedTimingVectors;
__thread TIMING_TYPE *detailedTimingVectorSize;
#endif


#ifdef INSTR_TIMING
inline void saveThreadDetailedTiming(std::stringstream &ss, HookContext *curContextPtr) {
    ss.str("");
    ss << scaler::ExtFuncCallHook::instance->folderName << "/threadDetailedTiming_" << curContextPtr->threadId
       << ".bin";

    //Calculate file total size

    ssize_t recordedInvocationCnt = 0;

    for (ssize_t i = 0; i < scaler::ExtFuncCallHook::instance->allExtSymbol.getSize(); ++i) {
        recordedInvocationCnt += detailedTimingVectorSize[i];
    }

    int fd;
    size_t realFileIdSizeInBytes = sizeof(ArrayDescriptor) +
                                   sizeof(ArrayDescriptor) * scaler::ExtFuncCallHook::instance->allExtSymbol.getSize()
                                   + recordedInvocationCnt * sizeof(TIMING_TYPE);

    uint8_t *fileContentInMem = nullptr;
    if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) {
        fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
    }
    uint8_t *_fileContentInMem = fileContentInMem;

    /*Write whole symbol info*/
    ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
    arrayDescriptor->arrayElemSize = 0;
    arrayDescriptor->arraySize = scaler::ExtFuncCallHook::instance->allExtSymbol.getSize();
    arrayDescriptor->magicNum = 167;
    fileContentInMem += sizeof(ArrayDescriptor);


    for (ssize_t i = 0; i < scaler::ExtFuncCallHook::instance->allExtSymbol.getSize(); ++i) {
        /**
         * Write array descriptor first
         */
        ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
        arrayDescriptor->arrayElemSize = sizeof(TIMING_TYPE);
        arrayDescriptor->arraySize = detailedTimingVectorSize[i];
        arrayDescriptor->magicNum = 167;
        fileContentInMem += sizeof(ArrayDescriptor);

        /**
         * Then write detailed timing array
         */
        memcpy(fileContentInMem, detailedTimingVectors[i], arrayDescriptor->arraySize * arrayDescriptor->arrayElemSize);
        fileContentInMem += arrayDescriptor->arraySize * arrayDescriptor->arrayElemSize;
    }
    if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) {
        fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
    }
}
#endif

bool dataSaveLock = false;


inline void savePerThreadTimingData(std::stringstream &ss, HookContext *curContextPtr) {
    ss.str("");
    ss << scaler::ExtFuncCallHook::instance->folderName << "/threadTiming_" << curContextPtr->threadId << ".bin";
    //INFO_LOGS("Saving timing data to %s", ss.str().c_str());

    int fd;
    size_t realFileIdSizeInBytes =
            sizeof(ThreadCreatorInfo) + sizeof(ArrayDescriptor) + curContextPtr->recArr->getSize() * sizeof(RecTuple);
    uint8_t *fileContentInMem = nullptr;
    if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) {
        fatalErrorS("Cannot fopen %s because:%s", ss.str().c_str(), strerror(errno));
    }
    uint8_t *_fileContentInMem = fileContentInMem;
    /**
     * Record who created the thread
     */
    ThreadCreatorInfo *threadCreatorInfo = reinterpret_cast<ThreadCreatorInfo *>(fileContentInMem);
    threadCreatorInfo->threadExecutionCycles = curContextPtr->endTImestamp - curContextPtr->startTImestamp;
    printf("End=%lu Start=%lu\n", curContextPtr->endTImestamp, curContextPtr->startTImestamp);
    threadCreatorInfo->threadCreatorFileId = curContextPtr->threadCreatorFileId;
    threadCreatorInfo->magicNum = 167;
    fileContentInMem += sizeof(ThreadCreatorInfo);

    /**
     * Record size information about the recorded array
     */
    ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
    arrayDescriptor->arrayElemSize = sizeof(RecTuple);
    arrayDescriptor->arraySize = curContextPtr->recArr->getSize();
    arrayDescriptor->magicNum = 167;
    fileContentInMem += sizeof(ArrayDescriptor);

//    for(int i=0;i<curContextPtr->recArr->getSize();++i){
//        if(curContextPtr->recArr->internalArr[i].count>0){
//            printf("%ld\n",curContextPtr->recArr->internalArr[i].count);
//        }
//    }
    /**
     * Write recording tuple onto the disk
     */
    memcpy(fileContentInMem, curContextPtr->recArr->data(),
           curContextPtr->recArr->getTypeSizeInBytes() * curContextPtr->recArr->getSize());

    if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) {
        fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
    }

    INFO_LOGS("Saving data to %s, %lu", scaler::ExtFuncCallHook::instance->folderName.c_str(), pthread_self());
}


inline void saveRealFileId(std::stringstream &ss, HookContext *curContextPtr) {
    ss.str("");
    ss << scaler::ExtFuncCallHook::instance->folderName << "/realFileId.bin";
    //The real id of each function is resolved in after hook, so I can only save it in datasaver

    int fd;
    ssize_t realFileIdSizeInBytes = sizeof(ArrayDescriptor) +
                                    (curContextPtr->_this->allExtSymbol.getSize()) * sizeof(uint64_t);
    uint8_t *fileContentInMem = nullptr;
    if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) {
        fatalErrorS(
                "Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
    }
    uint8_t *_fileContentInMem = fileContentInMem;

    /**
     * Write array descriptor first
     */
    ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
    arrayDescriptor->arrayElemSize = sizeof(uint64_t);
    arrayDescriptor->arraySize = curContextPtr->_this->allExtSymbol.getSize();
    arrayDescriptor->magicNum = 167;
    fileContentInMem += sizeof(ArrayDescriptor);

    uint64_t *realFileIdMem = reinterpret_cast<uint64_t *>(fileContentInMem);
    for (int i = 0; i < curContextPtr->_this->allExtSymbol.getSize(); ++i) {
        realFileIdMem[i] = curContextPtr->_this->pmParser.findExecNameByAddr(
                *(curContextPtr->_this->allExtSymbol[i].gotEntryAddr));
    }

    if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) {
        fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
    }
}


void saveData(HookContext *curContextPtr, bool finalize) {
    bypassCHooks = SCALER_TRUE;
    assert(curContextPtr != nullptr);

    /* CS: Check whether data has been saved. Make sure data is only saved once */
    if (__atomic_test_and_set(&curContextPtr->dataSaved, __ATOMIC_ACQUIRE)) {
        //INFO_LOGS("Thread data already saved, skip %d/%zd", i, threadContextMap.getSize());
        return;
    }
    /* CS: End */

    //Save per-thread end time
    threadTerminatedRecord(curContextPtr);

    std::stringstream ss;

#ifdef INSTR_TIMING
    //Save per-thread detailed timing
    saveThreadDetailedTiming(ss, curContextPtr);
#endif

    //Save per-thread timing data
    savePerThreadTimingData(ss, curContextPtr);

    //Save other thread if this thread is main thread and is exiting
    if (curContextPtr->isMainThread || finalize) {
        saveRealFileId(ss, curContextPtr);

        for (int i = 0; i < threadContextMap.getSize(); ++i) {
            HookContext *threadContext = threadContextMap[i];
            saveData(threadContext);
        }
    }

}

DataSaver::~DataSaver() {
    saveData(curContext);
}
