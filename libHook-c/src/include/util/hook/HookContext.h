#ifndef HOOK_CONTEXT_H
#define HOOK_CONTEXT_H

#include <util/datastructure/FStack.h>
#include <cstdio>
#include <type/InvocationTree.h>
#include <util/tool/Timer.h>
#include <type/RecTuple.h>
#include <atomic>
#include "ExtFuncCallHook.h"

extern "C" {

static uint64_t testA = 0;
static uint64_t *testAddr = 0;
#define MAX_CALL_DEPTH 64 //N+1 because of dummy variable

struct HookTuple {
    uint64_t callerAddr; //8
    uint64_t logicalClockCycles; //8
    int64_t symId; //8
    uint32_t clockTicks; //8
};

struct HookContext {
    //todo: Initialize using maximum stack size
    int64_t indexPosi;//8bytes
    scaler::Array<RecTuple> *recArr; //8bytes
    //Records which function calls which function for how long, the index is scalerid (Only contains hooked function)
    //todo: Replace timingMatrix to a class
    int64_t threadCreatorFileId = 1; //Which library created the current thread? The default one is main thread
    scaler::ExtFuncCallHook *_this = nullptr; //8bytes
    //Records which symbol is called for how many times, the index is scalerid (Only contains hooked function)
    uint64_t threadExecTime; //Used for application time attribution
    //New cacheline
    uint64_t cachedWallClockSnapshot;
    uint64_t cachedLogicalClock;
    uint32_t cachedThreadNum;
    //Variables used to determine whether it's called by hook handler or not
    HookTuple hookTuple[MAX_CALL_DEPTH]; //8bytes aligned
    pthread_t threadId;
    uint8_t dataSaved = false;
    uint8_t isMainThread = false;
    uint8_t initialized = 0;
};

const uint8_t SCALER_TRUE = 145;
const uint8_t SCALER_FALSE = 167;
extern uint32_t threadNum; //The current number of threads
extern std::atomic<uint64_t> wallclockSnapshot; //Used as update timestamp
extern uint64_t logicalClock;

class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver();
};


void saveData(HookContext *context, bool finalize = false);


extern __thread HookContext *curContext;

extern __thread uint8_t bypassCHooks; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE

//extern scaler::SymID pthreadCreateSymId;

extern scaler::Vector<HookContext *> threadContextMap;

extern pthread_mutex_t threadDataSavingLock;

bool initTLS();


}
#endif