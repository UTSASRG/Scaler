#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>

extern "C" {

HookContext *constructContext(ssize_t libFileSize, ssize_t hookedSymbolSize) {

    HookContext *rlt = new HookContext();
    rlt->timingArr = new scaler::Array<uint64_t>(hookedSymbolSize);
    rlt->timingArr->setSize(hookedSymbolSize);
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
    rlt->hookTuple[rlt->indexPosi].timeStamp = 0;
    rlt->hookTuple[rlt->indexPosi].symId = 0;
    rlt->indexPosi = 1;
    assert(scaler::ExtFuncCallHook::instance != nullptr);
    rlt->_this = scaler::ExtFuncCallHook::instance;

    saverElem.initializeMe = 1; //Force initialize tls

    return rlt;
}

bool initTLS() {
    assert(scaler::ExtFuncCallHook::instance != nullptr);

    //Put a dummy variable to avoid null checking
    //Initialize saving data structure
    curContext = constructContext(
            scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
            scaler::ExtFuncCallHook::instance->allExtSymbol.getSize() + 1);


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
    saveData();
}

void saveData() {
    bypassCHooks = SCALER_TRUE;
    HookContext *curContextPtr = curContext;

    if (curContextPtr->dataSaved) {
        DBG_LOG("Data already saved for this thread");
        return;
    }
    curContextPtr->dataSaved = true;

    if (!curContextPtr->endTImestamp) {
        //Not finished succesfully
        curContextPtr->endTImestamp = getunixtimestampms();
    }

    if (!curContext) {
        fatalError("curContext is not initialized, won't save anything");
        return;
    }
    std::stringstream ss;
    ss << scaler::ExtFuncCallHook::instance->folderName << "/threadTiming_" << pthread_self() << ".bin";
    //INFO_LOGS("Saving timing data to %s", ss.str().c_str());
    FILE *threadDataSaver = fopen(ss.str().c_str(), "wb");
    if (!threadDataSaver) {
        fatalErrorS("Cannot fopen %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }

    //Main application at the end
    curContextPtr->timingArr->internalArr[curContextPtr->timingArr->getSize() - 1] =
            curContextPtr->endTImestamp - curContextPtr->startTImestamp;


    if (fwrite(&curContextPtr->curFileId, sizeof(HookContext::curFileId), 1, threadDataSaver) != 1) {
        fatalErrorS("Cannot curFileId of %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }

    int64_t timeEntrySize = curContextPtr->timingArr->getSize();
    if (fwrite(&timeEntrySize, sizeof(int64_t), 1, threadDataSaver) != 1) {
        fatalErrorS("Cannot write timeEntrySize of %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }
    if (fwrite(curContextPtr->timingArr->data(), curContextPtr->timingArr->getTypeSizeInBytes(),
               curContextPtr->timingArr->getSize(), threadDataSaver) !=
        curContextPtr->timingArr->getSize()) {
        fatalErrorS("Cannot write timingArr of %s because:%s", ss.str().c_str(),
                    strerror(errno));
    }
    INFO_LOGS("Saving data to %s, %lu", scaler::ExtFuncCallHook::instance->folderName.c_str(), pthread_self());

    if (curContextPtr->isMainThread) {
        ss.str("");
        ss << scaler::ExtFuncCallHook::instance->folderName << "/realFileId.bin";
        //The real id of each function is resolved in after hook, so I can only save it in datasaver

        int fd;

        size_t realFileIdSizeInBytes = (curContextPtr->_this->allExtSymbol.getSize() + 1) * sizeof(ssize_t);
        size_t *realFileIdMem = nullptr;
        if (!scaler::fOpen4Write<size_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, realFileIdMem)) {
            fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }

        for (int i = 0; i < curContextPtr->_this->allExtSymbol.getSize(); ++i) {
            realFileIdMem[i] = curContextPtr->_this->pmParser.findExecNameByAddr(
                    *(curContextPtr->_this->allExtSymbol[i].gotEntryAddr));
        }
        if (!scaler::fClose<size_t>(fd, realFileIdSizeInBytes, realFileIdMem)) {
            fatalError("Cannot close file");
        }
    }


    fclose(threadDataSaver);
}

}


