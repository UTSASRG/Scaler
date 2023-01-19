#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <cxxabi.h>
#include <type/RecTuple.h>
#include "util/hook/LogicalClock.h"
#include "util/hook/DataSaver.h"


thread_local DataSaver saverElem; //To be called

HookContext *
constructContext(ssize_t libFileSize, ssize_t hookedSymbolSize, scaler::Array<scaler::ExtSymInfo> &allExtSymbol) {

    uint8_t *contextHeap = static_cast<uint8_t *>(mmap(NULL, sizeof(HookContext) +
                                                             sizeof(scaler::Array<uint64_t>) +
                                                             sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
                                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
//    INFO_LOGS("Context size=%lu %p", sizeof(HookContext) +
//                                     sizeof(scaler::Array<uint64_t>) +
//                                     sizeof(pthread_mutex_t), &testA);
    HookContext *rlt = reinterpret_cast<HookContext *>(contextHeap);
    assert(rlt != nullptr);
    memset(rlt, 0, sizeof(HookContext) + sizeof(scaler::Array<RecTuple>) + sizeof(scaler::Array<RecTuple>));
    rlt->recArr = new(contextHeap + sizeof(HookContext)) scaler::Array<RecTuple>(hookedSymbolSize);
#ifdef INSTR_TIMING
    detailedTimingVectors = new TIMING_TYPE *[hookedSymbolSize];
    detailedTimingVectorSize = new TIMING_TYPE[hookedSymbolSize];
    memset(detailedTimingVectorSize, 0, sizeof(TIMING_TYPE) * hookedSymbolSize);
    for (ssize_t i = 0; i < hookedSymbolSize; ++i) {
        detailedTimingVectors[i] = new TIMING_TYPE[TIMING_REC_COUNT];
        memset(detailedTimingVectors[i], 0, sizeof(TIMING_TYPE) * TIMING_REC_COUNT);
    }
#endif

    rlt->recArr->setSize(hookedSymbolSize);
    //Initialize gap to one
    for (int i = 0; i < rlt->recArr->getSize(); ++i) {
        //number mod 2^n is equivalent to stripping off all but the n lowest-order
        rlt->recArr->internalArr[i].gap = allExtSymbol[i].initialGap; //0b11 if %4, because 4=2^2 Initially time everything
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

    curContext = constructContext(scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
                                  scaler::ExtFuncCallHook::instance->allExtSymbol.getSize(),
                                  scaler::ExtFuncCallHook::instance->allExtSymbol);
//#ifdef PRINT_DBG_LOG
    printRecOffset();
//#endif
    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }
    //RuntimeInfo newInfo;
    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE



