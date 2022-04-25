#ifndef THREAD_LOCAL_H
#define THREAD_LOCAL_H

#include <util/datastructure/FStack.h>
#include <cstdio>
#include <type/InvocationTree.h>
#include "ExtFuncCallHook.h"

extern "C" {


#define MAX_CALL_DEPTH 64 //N+1 because of dummy variable

struct HookTuple {
    uint64_t callerAddr;
    int64_t timeStamp;
    int64_t symId;
};

struct HookContext {
    //todo: Initialize using maximum stack size
    int64_t indexPosi;//8bytes
    scaler::Array<uint64_t> *timingArr; //8bytes
    //Records which function calls which function for how long, the index is scalerid (Only contains hooked function)
    //todo: Replace timingMatrix to a class
    int64_t curFileId = 1; //The default one is main thread 8bytes
    scaler::ExtFuncCallHook *_this = nullptr; //8bytes
    //Records which symbol is called for how many times, the index is scalerid (Only contains hooked function)
    uint8_t isMainThread = false;
    uint8_t dataSaved = false;
    uint8_t initialized = 0;
    uint8_t pad1 = 0;
    uint8_t pad2 = 0;
    uint8_t pad3 = 0;
    uint8_t pad4 = 0;
    uint8_t pad5 = 0;
    uint64_t startTImestamp;
    uint64_t endTImestamp;
    uint64_t *realFileIdArr = nullptr; //Used to record real id
    //New cacheline
    //Variables used to determine whether it's called by hook handler or not
    HookTuple hookTuple[MAX_CALL_DEPTH]; //8bytes aligned
};

const uint8_t SCALER_TRUE = 145;
const uint8_t SCALER_FALSE = 167;


class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver();
};

void saveData();

static thread_local DataSaver saverElem;

extern __thread HookContext *curContext;

extern __thread uint8_t bypassCHooks; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE



bool initTLS();
}
#endif