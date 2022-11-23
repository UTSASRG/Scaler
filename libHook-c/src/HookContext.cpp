#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <cxxabi.h>
#include <type/RecTuple.h>

extern "C" {
static thread_local DataSaver saverElem;

HookContext *constructContext(ssize_t libFileSize, ssize_t hookedSymbolSize) {

    uint8_t *contextHeap = static_cast<uint8_t *>(mmap(NULL, sizeof(HookContext) +
                                                             sizeof(scaler::Array<uint64_t>) +
                                                             sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
                                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
//    INFO_LOGS("Context size=%lu %p", sizeof(HookContext) +
//                                     sizeof(scaler::Array<uint64_t>) +
//                                     sizeof(pthread_mutex_t), &testA);
    HookContext *rlt = reinterpret_cast<HookContext *>(contextHeap);
    assert(rlt != nullptr);
    memset(rlt, 0, sizeof(HookContext) + sizeof(scaler::Array<RecTuple>) + sizeof(pthread_mutex_t));
    rlt->recArr = new(contextHeap + sizeof(HookContext)) scaler::Array<RecTuple>(hookedSymbolSize);
    rlt->threadDataSavingLock = reinterpret_cast<pthread_mutex_t *>(contextHeap + sizeof(HookContext) +
                                                                    sizeof(scaler::Array<uint64_t>));
#ifdef INSTR_TIMING
    detailedTimingVectors = new TIMING_TYPE *[hookedSymbolSize];
    detailedTimingVectorSize = new TIMING_TYPE[hookedSymbolSize];
    memset(detailedTimingVectorSize, 0, sizeof(TIMING_TYPE) * hookedSymbolSize);
    for (ssize_t i = 0; i < hookedSymbolSize; ++i) {
        detailedTimingVectors[i] = new TIMING_TYPE[TIMING_REC_COUNT];
        memset(detailedTimingVectors[i], 0, sizeof(TIMING_TYPE) * TIMING_REC_COUNT);
    }
#endif

    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(rlt->threadDataSavingLock, &Attr);
    rlt->recArr->setSize(hookedSymbolSize);
    //Initialize gap to one
    for (int i = 0; i < rlt->recArr->getSize(); ++i) {
        //number mod 2^n is equivalent to stripping off all but the n lowest-order
        rlt->recArr->internalArr[i].gap = 0; //0b11 if %4, because 4=2^2 Initially time everything
        rlt->recArr->internalArr[i].count = 0;
    }
//    memArrayHeap(1), timingArr(hookedSymbolSize),
//            indexPosi(0)

    //    //Initialize root node
//    rootNode = memArrayHeap.allocArr(1);
//    //Initialize root node child size. id=0 is always the main application
//    rootNode->childrenSize = scaler_extFuncCallHookAsm_thiz->elfImgInfoMap[0].hookedSymbolSize;
//    //Initialize children
//    rootNode->children = memArrayHeap.allocArr(rootNode->childrenSize);
//    rootNode->scalerId = -1; //Indiacate root node
    rlt->initialized = SCALER_TRUE;

    //Push a dummy value in the stack (Especially for callAddr, because we need to avoid null problem)
    rlt->hookTuple[rlt->indexPosi].callerAddr = 0;
    rlt->hookTuple[rlt->indexPosi].clockCycles = 0;
    rlt->hookTuple[rlt->indexPosi].symId = 0;
    rlt->indexPosi = 1;
    assert(scaler::ExtFuncCallHook::instance != nullptr);
    rlt->_this = scaler::ExtFuncCallHook::instance;

    rlt->threadId = pthread_self();
    saverElem.initializeMe = 1; //Force initialize tls

    return rlt;
}

bool destructContext() {
    HookContext *curContextPtr = curContext;
    delete curContextPtr->recArr;
    munmap(curContextPtr, sizeof(HookContext) +
                          sizeof(scaler::Array<uint64_t>) +
                          sizeof(pthread_mutex_t));
    curContext = nullptr;
}


/**
 * This code should be put into plt entry, but plt entry have no more space.
 * 32bytes aligned
 */
void __attribute__((used, noinline, optimize(3))) printRecOffset() {

    auto i __attribute__((used)) = (uint8_t *) curContext;
    auto j __attribute__((used)) = (uint8_t *) &curContext->recArr->internalArr;

    auto k __attribute__((used)) = (uint8_t *) &curContext->recArr->internalArr[0];
    auto l __attribute__((used)) = (uint8_t *) &curContext->recArr->internalArr[0].count;
    auto m __attribute__((used)) = (uint8_t *) &curContext->recArr->internalArr[0].gap;

    printf("\nTLS offset: Check assembly\n"
             "RecArr Offset: 0x%lx\n"
             "Counting Entry Offset: 0x%lx\n"
             "Gap Entry Offset: 0x%lx\n", j - i, l - k, m - k);
}


void __attribute__((used, noinline, optimize(3))) accessTLS1() {
    HookContext *contextPtr __attribute__((used)) = curContext;
    auto &j __attribute__((used)) = contextPtr->recArr;
    auto &k __attribute__((used)) = contextPtr->recArr->internalArr;
    printf("%p.%p,%p", contextPtr, j, k);


}

bool initTLS() {
    assert(scaler::ExtFuncCallHook::instance != nullptr);

    //Put a dummy variable to avoid null checking
    //Initialize saving data structure
    curContext = constructContext(
            scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
            scaler::ExtFuncCallHook::instance->allExtSymbol.getSize() + 1);
#ifdef PRINT_DBG_LOG
    printRecOffset();
#endif
    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }
    //RuntimeInfo newInfo;
    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE


const int TIMING_REC_COUNT = 20000;
typedef int64_t TIMING_TYPE;
__thread TIMING_TYPE **detailedTimingVectors;
__thread TIMING_TYPE *detailedTimingVectorSize;

DataSaver::~DataSaver() {
    saveData(curContext);
}


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

inline void saveDataForAllOtherThread(std::stringstream &ss, HookContext *curContextPtr) {
    INFO_LOG("Save data of all existing threads");
    for (int i = 0; i < threadContextMap.getSize(); ++i) {
        HookContext *threadContext = threadContextMap[i];
        if (!threadContext->dataSaved) {
            pthread_mutex_lock(threadContext->threadDataSavingLock);
            INFO_LOGS("Thread data not saved, save it %d/%zd", i, threadContextMap.getSize());
            saveData(threadContext);
            pthread_mutex_unlock(threadContext->threadDataSavingLock);
        } else {
            INFO_LOGS("Thread data already saved, skip %d/%zd", i, threadContextMap.getSize());
        }
    }
}

void saveData(HookContext *curContextPtr, bool finalize) {
    bypassCHooks = SCALER_TRUE;
    if (!curContextPtr) {
        curContextPtr = curContext;
    }

    pthread_mutex_lock(curContextPtr->threadDataSavingLock);

    if (curContextPtr->dataSaved) {
        DBG_LOG("Data already saved for this thread");
        pthread_mutex_unlock(curContextPtr->threadDataSavingLock);
        return;
    }
    curContextPtr->dataSaved = true;

    //Resolve real address
    if (!curContextPtr->endTImestamp) {
        //Not finished succesfully
        curContextPtr->endTImestamp = getunixtimestampms();
    }

    if (!curContext) {
        fatalError("curContext is not initialized, won't save anything");
        return;
    }
    std::stringstream ss;

#ifdef INSTR_TIMING
    saveThreadDetailedTiming(ss, curContextPtr);
#endif

    savePerThreadTimingData(ss, curContextPtr);

    if (curContextPtr->isMainThread || finalize) {
        saveRealFileId(ss, curContextPtr);
        saveDataForAllOtherThread(ss, curContextPtr);
    }

    pthread_mutex_unlock(curContextPtr->threadDataSavingLock);

}

}


