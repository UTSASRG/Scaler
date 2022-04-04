#ifndef THREAD_LOCAL_H
#define THREAD_LOCAL_H

#include <util/datastructure/FStack.h>
#include <cstdio>
#include <type/InvocationTree.h>
#include "ExtFuncCallHook.h"

extern "C" {


#define MAX_CALL_DEPTH 11 //10+1 because of dummy variable

class HookContext {
public:
    //todo: Initialize using maximum stack size
    scaler::FStack<scaler::SymID, MAX_CALL_DEPTH> symId;

    //Variables used to determine whether it's called by hook handler or not
    scaler::FStack<void *, MAX_CALL_DEPTH> callerAddr;
    scaler::FStack<long long, MAX_CALL_DEPTH> timeStamp;
    //Records which function calls which function for how long, the index is scalerid (Only contains hooked function)
    //todo: Replace timingMatrix to a class
    int64_t curThreadNumber = 1; //The default one is main thread

    //Records which symbol is called for how many times, the index is scalerid (Only contains hooked function)
    bool isThreadCratedByMyself = false;
    bool threadTerminatedPeacefully = false;
    short initialized = 0;

    scaler::InvocationTree *rootNode = nullptr;
    scaler::InvocationTree *curNode = nullptr;
    short curNodeLevel = 0; //Which level is curNode. This number is used to help afterhook to move curNode to the correct level and record. This is necessary since we don't know symbol address in prehook.
    scaler::MemoryHeapArray<scaler::InvocationTree> memArrayHeap;

    HookContext(ssize_t libFileSize, ssize_t hookedSymbolSize);

    ~HookContext();
};

const uint8_t SCALER_TRUE = 145;
const uint8_t SCALER_FALSE = 167;

extern __thread HookContext *curContext;

extern __thread uint8_t bypassCHooks; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE

bool initTLS();
}
#endif