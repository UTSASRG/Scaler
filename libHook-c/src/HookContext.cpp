#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <cxxabi.h>

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


    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(rlt->threadDataSavingLock, &Attr);
    rlt->recArr->setSize(hookedSymbolSize);
    //Initialize gap to one
    for (int i = 0; i < rlt->recArr->getSize(); ++i) {
        //number mod 2^n is equivalent to stripping off all but the n lowest-order
        rlt->recArr->internalArr[i].gap = 0; //0x11 if %4, because 4=2^2 Initially time everything
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

    DBG_LOGS("\nTLS offset: Check assembly\n"
             "RecArr Offset: 0x%x\n"
             "Counting Entry Offset: 0x%x\n"
             "Gap Entry Offset: 0x%x\n", j - i, l - k, m - k);
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

DataSaver::~DataSaver() {
    saveData(curContext);
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
        INFO_LOGS("Thread not finished succesfully, record an end timestamp %lu", curContextPtr->endTImestamp);
    }

    if (!curContext) {
        fatalError("curContext is not initialized, won't save anything");
        return;
    }
    std::stringstream ss;
    ss << scaler::ExtFuncCallHook::instance->folderName << "/threadTiming_" << curContextPtr->threadId << ".bin";
    //INFO_LOGS("Saving timing data to %s", ss.str().c_str());
    FILE *threadDataSaver = fopen(ss.str().c_str(), "wb");
    if (!threadDataSaver) {
        fatalErrorS("Cannot fopen %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }

    //Main application at the end
    curContextPtr->recArr->internalArr[curContextPtr->recArr->getSize() - 1].totalClockCycles =
            curContextPtr->endTImestamp - curContextPtr->startTImestamp;
    INFO_LOGS("Total cycles=%lu\n",
           curContextPtr->recArr->internalArr[curContextPtr->recArr->getSize() - 1].totalClockCycles);

    if (fwrite(&curContextPtr->curFileId, sizeof(HookContext::curFileId), 1, threadDataSaver) != 1) {
        fatalErrorS(
                "Cannot curFileId of %s because:%s", ss.str().c_str(),
                strerror(errno));
    }

    int64_t timeEntrySize = curContextPtr->recArr->getSize();
    if (fwrite(&timeEntrySize, sizeof(int64_t), 1, threadDataSaver) != 1) {
        fatalErrorS(
                "Cannot write timeEntrySize of %s because:%s", ss.str().c_str(),
                strerror(errno));
    }
    if (fwrite(curContextPtr->recArr->data(), curContextPtr->recArr->getTypeSizeInBytes(),
               curContextPtr->recArr->getSize(), threadDataSaver) !=
        curContextPtr->recArr->getSize()) {
        fatalErrorS("Cannot write timingArr of %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }


    INFO_LOGS("Thread %lu is saving data to %s", pthread_self(), scaler::ExtFuncCallHook::instance->folderName.c_str());

    if (curContextPtr->isMainThread || finalize) {
        ss.str("");
        ss << scaler::ExtFuncCallHook::instance->folderName << "/realFileId.bin";
        //The real id of each function is resolved in after hook, so I can only save it in datasaver

        int fd;

        size_t realFileIdSizeInBytes = (curContextPtr->_this->allExtSymbol.getSize() + 1) * sizeof(ssize_t);
        size_t *realFileIdMem = nullptr;
        if (!scaler::fOpen4Write<size_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, realFileIdMem)) {
            fatalErrorS(
                    "Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }

        for (int i = 0; i < curContextPtr->_this->allExtSymbol.getSize(); ++i) {
            realFileIdMem[i] = curContextPtr->_this->pmParser.findExecNameByAddr(
                    *(curContextPtr->_this->allExtSymbol[i].gotEntryAddr));
        }
        if (!scaler::fClose<size_t>(fd, realFileIdSizeInBytes, realFileIdMem)) {
            fatalError("Cannot close file");
        }

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

    fclose(threadDataSaver);
    pthread_mutex_unlock(curContextPtr->threadDataSavingLock);

}

}


