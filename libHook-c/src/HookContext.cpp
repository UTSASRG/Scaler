#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <cxxabi.h>
#include <type/RecTuple.h>
#include "util/tool/AtomicSpinLock.h"
#include <util/hook/LogicalClock.h>


uint64_t logicalClock;
std::atomic<uint64_t> wallclockSnapshot;
uint64_t updateSpinlock;
AtomicSpinLock threadUpdateLock; //Lock used in LogicalClock.h to update thread counters

extern "C" {
static thread_local DataSaver saverElem;
uint32_t threadNum = 0; //Actual thread number recorded

HookContext *constructContext(scaler::ExtFuncCallHook &inst) {
    HookContext *rlt = new HookContext();
    if (!rlt) {
        fatalError("Cannot allocate memory for HookContext")
    }
    rlt->ldArr = new scaler::FixedArray<scaler::Array<RecTuple>>(1024);//A maximum of 1024 dynamic loaded libraries
    rlt->ldArr->allocate(inst.allExtSymbol.getSize());
    assert(inst.elfImgInfoMap.getSize() == inst.allExtSymbol.getSize());
    //No contention because parent function will acquire a lock
    //Allocate recArray
    for (ssize_t loadingId = 0; loadingId < rlt->ldArr->getSize(); ++loadingId) {
        rlt->ldArr[0][loadingId].allocate(
                inst.allExtSymbol[loadingId].getSize());//I wrote [0] rather than * to deference pointer so the format is cleaner
        //Initialize gap to one
        for (ssize_t symId = 0; symId < inst.allExtSymbol[loadingId].getSize(); ++symId) {
            //number mod 2^n is equivalent to stripping off all but the n lowest-order
            rlt->ldArr[0][loadingId][symId].gap = inst.allExtSymbol[loadingId][symId].initialGap;//0b11 if %4, because 4=2^2 Initially time everything
            rlt->ldArr[0][loadingId][symId].count = 0;
        }
    }
    rlt->initialized = SCALER_TRUE;

    //Push a dummy value in the stack (Especially for callAddr, because we need to avoid null problem)
    rlt->hookTuple[rlt->indexPosi].callerAddr = 0;
    rlt->hookTuple[rlt->indexPosi].logicalClockCycles = 0;
    rlt->hookTuple[rlt->indexPosi].symId = 0;
    rlt->indexPosi = 1;

    __atomic_store_n(&rlt->dataSaved, false, __ATOMIC_RELEASE);

    assert(scaler::ExtFuncCallHook::instance != nullptr);
    rlt->_this = scaler::ExtFuncCallHook::instance;

    rlt->threadId = pthread_self();
    saverElem.initializeMe = 1; //Force initialize tls
    return rlt;
}

bool destructContext() {
    HookContext *curContextPtr = curContext;
    delete curContextPtr->ldArr;
    munmap(curContextPtr, sizeof(HookContext) +
                          sizeof(scaler::Array<uint64_t>) +
                          sizeof(pthread_mutex_t));
    curContext = nullptr;
}


void __attribute__((used, noinline, optimize(3))) printRecOffset() {

    auto i __attribute__((used)) = (uint8_t *) curContext;
    auto j __attribute__((used)) = (uint8_t *) &curContext->ldArr;

    auto n __attribute__((used)) = (uint8_t *) curContext->ldArr;
    auto o __attribute__((used)) = (uint8_t *) &(curContext->ldArr->internalArr);

    auto k __attribute__((used)) = (uint8_t *) &curContext->ldArr[0][0].internalArr[0];
    auto l __attribute__((used)) = (uint8_t *) &curContext->ldArr[0][0].internalArr[0].count;
    auto m __attribute__((used)) = (uint8_t *) &curContext->ldArr[0][0].internalArr[0].gap;

    printf("\nTLS offset: Check assembly\n"
           "Context.ldArr Offset: 0x%lx\n"
           "Array<...>->internalArr Offset: 0x%lx\n"
           "Counting Entry Offset: 0x%lx\n"
           "Gap Entry Offset: 0x%lx\n", j - i, o-n, l - k, m - k);
}


void __attribute__((used, noinline, optimize(3))) accessTLS1() {
    HookContext *contextPtr __attribute__((used)) = curContext;
    auto &j __attribute__((used)) = contextPtr->ldArr;
    auto &k __attribute__((used)) = contextPtr->ldArr->internalArr;
    printf("%p.%p,%p", contextPtr, j, k);


}

bool initTLS() {
    assert(scaler::ExtFuncCallHook::instance != nullptr);
    scaler::ExtFuncCallHook &inst = *scaler::ExtFuncCallHook::getInst();
    pthread_mutex_lock(&(inst.dynamicLoadingLock));

    //Put a dummy variable to avoid null checking
    //Initialize saving data structure
    curContext = constructContext(inst);

//#ifdef PRINT_DBG_LOG
    printRecOffset();
//#endif

    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }
    pthread_mutex_unlock(&inst.dynamicLoadingLock);

    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE

DataSaver::~DataSaver() {
    saveData(curContext);
}

inline void savePerThreadTimingData(std::stringstream &ss, HookContext *curContextPtr) {
    ss.str("");
    ss << scaler::ExtFuncCallHook::instance->folderName << "/threadTiming_" << curContextPtr->threadId << ".bin";
    //INFO_LOGS("Saving timing data to %s", ss.str().c_str());

    int fd;
    size_t realFileIdSizeInBytes =
            sizeof(ThreadCreatorInfo) + sizeof(ArrayDescriptor) + curContextPtr->ldArr->getSize() * sizeof(RecTuple);
    uint8_t *fileContentInMem = nullptr;
    if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) {
        fatalErrorS("Cannot fopen %s because:%s", ss.str().c_str(), strerror(errno));
    }
    uint8_t *_fileContentInMem = fileContentInMem;
    /**
     * Record who created the thread
     */
    ThreadCreatorInfo *threadCreatorInfo = reinterpret_cast<ThreadCreatorInfo *>(fileContentInMem);
    threadCreatorInfo->threadExecutionCycles = curContextPtr->threadExecTime;
    threadCreatorInfo->threadCreatorFileId = curContextPtr->threadCreatorFileId;
    threadCreatorInfo->magicNum = 167;
    fileContentInMem += sizeof(ThreadCreatorInfo);

    /**
     * Record size information about the recorded array
     */
    ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
    arrayDescriptor->arrayElemSize = sizeof(RecTuple);
    arrayDescriptor->arraySize = curContextPtr->ldArr->getSize();
    arrayDescriptor->magicNum = 167;
    fileContentInMem += sizeof(ArrayDescriptor);

//    for(int i=0;i<curContextPtr->ldArr->getSize();++i){
//        if(curContextPtr->ldArr->internalArr[i].count>0){
//            printf("%ld\n",curContextPtr->ldArr->internalArr[i].count);
//        }
//    }
    /**
     * Write recording tuple onto the disk
     */
    memcpy(fileContentInMem, curContextPtr->ldArr->data(),
           curContextPtr->ldArr->getTypeSizeInBytes() * curContextPtr->ldArr->getSize());

    if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) {
        fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
    }

    DBG_LOGS("Saving data to %s, %lu", scaler::ExtFuncCallHook::instance->folderName.c_str(), pthread_self());
}

//inline void saveApiInvocTimeByLib(std::stringstream &ss, HookContext *curContextPtr){
//    ss.str("");
//    ss << scaler::ExtFuncCallHook::instance->folderName << "/apiInvocTimeByLib_"<< curContextPtr->threadId << ".bin";
//    //The real id of each function is resolved in after hook, so I can only save it in datasaver
//
//    int fd;
//    ssize_t selfTimeSizeInBytes = sizeof(ArrayDescriptor) + (curContextPtr->selfTimeArr->getSize()) * sizeof(uint64_t);
//    uint8_t *fileContentInMem = nullptr;
//    if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, selfTimeSizeInBytes, fileContentInMem)) {
//        fatalErrorS(
//                "Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
//    }
//    uint8_t *_fileContentInMem = fileContentInMem;
//
//    /**
//     * Write array descriptor first
//     */
//    ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
//    arrayDescriptor->arrayElemSize = sizeof(uint64_t);
//    arrayDescriptor->arraySize = curContextPtr->selfTimeArr->getSize();
//    arrayDescriptor->magicNum = 167;
//    fileContentInMem += sizeof(ArrayDescriptor);
//
//    uint64_t *realFileIdMem = reinterpret_cast<uint64_t *>(fileContentInMem);
//    for (int i = 0; i < curContextPtr->selfTimeArr->getSize(); ++i) {
//        realFileIdMem[i] = curContextPtr->selfTimeArr->internalArr[i];
//    }
//
//    if (!scaler::fClose<uint8_t>(fd, selfTimeSizeInBytes, _fileContentInMem)) {
//        fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
//    }
//}

inline void saveRealFileId(std::stringstream &ss, HookContext *curContextPtr) {
    for (ssize_t curLoadingId = 0; curLoadingId < curContextPtr->_this->allExtSymbol.getSize(); ++curLoadingId) {
        ss.str("");
        ss << scaler::ExtFuncCallHook::instance->folderName << "/" << curLoadingId << "_realFileId.bin";
        //The real id of each function is resolved in after hook, so I can only save it in datasaver

        int fd;
        ssize_t realFileIdSizeInBytes = sizeof(ArrayDescriptor) +
                                        (curContextPtr->_this->allExtSymbol[curLoadingId].getSize()) * sizeof(uint64_t);
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
        arrayDescriptor->arraySize = curContextPtr->_this->allExtSymbol[curLoadingId].getSize();
        arrayDescriptor->magicNum = 167;
        fileContentInMem += sizeof(ArrayDescriptor);

        uint64_t *realFileIdMem = reinterpret_cast<uint64_t *>(fileContentInMem);
        for (int i = 0; i < curContextPtr->_this->allExtSymbol[curLoadingId].getSize(); ++i) {
            realFileIdMem[i] = curContextPtr->_this->pmParser.findFileIdByAddr(
                    *(curContextPtr->_this->allExtSymbol[curLoadingId][i].gotEntryAddr));
        }

        if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) {
            fatalErrorS("Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
        }
    }
}

inline void saveDataForAllOtherThread(std::stringstream &ss, HookContext *curContextPtr) {
    DBG_LOG("Save data of all existing threads");
    for (int i = 0; i < threadContextMap.getSize(); ++i) {
        HookContext *threadContext = threadContextMap[i];
        saveData(threadContext);
    }
}


void saveData(HookContext *curContextPtr, bool finalize) {
    bypassCHooks = SCALER_TRUE;

    /* CS: Check whether data has been saved. Make sure data is only saved once */
    if (__atomic_test_and_set(&curContextPtr->dataSaved, __ATOMIC_ACQUIRE)) {
        //INFO_LOGS("Thread data already saved, skip %d/%zd", i, threadContextMap.getSize());
        return;
    }
    /* CS: End */



    if (!curContext) {
        fatalError("curContext is not initialized, won't save anything");
        return;
    }

    uint64_t curLogicalClock = threadTerminatedRecord();
//    INFO_LOGS("AttributingThreadEndTime+= %lu - %lu", curLogicalClock, curContextPtr->threadExecTime);
    curContextPtr->threadExecTime = curLogicalClock -
                                    curContextPtr->threadExecTime; //curContextPtr->threadExecTime is set to logical clock in the beginning


    std::stringstream ss;

    savePerThreadTimingData(ss, curContextPtr);
//    saveApiInvocTimeByLib(ss, curContextPtr);

    if (curContextPtr->isMainThread || finalize) {
//        INFO_LOGS("Data saved to %s", scaler::ExtFuncCallHook::instance->folderName.c_str());
        saveRealFileId(ss, curContextPtr);
        saveDataForAllOtherThread(ss, curContextPtr);
    }

}

}


