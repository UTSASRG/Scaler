#include <util/hook/proxy/pthreadProxy.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>

extern "C" {

typedef void (*pthread_exit_origt)(void *__retval);
typedef int (*pthread_create_origt)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
typedef int (*pthread_cancel_origt)(pthread_t __th);
typedef int (*pthread_cancel_origt)(pthread_t __th);
typedef int (*pthread_join_origt)(pthread_t __th, void **__thread_return);

pthread_create_origt pthread_create_orig;
pthread_join_origt pthread_join_orig;

// Define structure to hold arguments for dummy thread function
struct dummy_thread_function_args {
    void *(*actual_thread_function)(void *data);

    void *pthreadCreateRetAddr;
    void *data;
};


// Define the dummy thread function
// Entering this function means the thread has been successfully created
// Instrument thread beginning, call the original thread function, instrument thread end
void *dummy_thread_function(void *data) {
    __atomic_add_fetch(&threadNum, 1, __ATOMIC_RELAXED);
    __atomic_add_fetch(&threadNumPhase, 1, __ATOMIC_RELAXED);

    /**
     * Perform required actions at beginning of thread
     */
    initTLS();

    /**
     * Call actual thread function
     */
    // Extract arguments and call actual thread function
    auto *args = static_cast<dummy_thread_function_args *>(data);
    void *argData = args->data;
    auto actualFuncPtr = args->actual_thread_function;
    void *pthreadCreateRetAddr = args->pthreadCreateRetAddr;
    free(args);
    args = nullptr;

    HookContext *curContextPtr = curContext;
    assert(curContextPtr != NULL);
    curContextPtr->threadCreatorFileId = curContextPtr->_this->pmParser.findExecNameByAddr(
            pthreadCreateRetAddr);
    DBG_LOGS("Thread is created by %ld", curContextPtr->_this->pmParser.findExecNameByAddr(
            pthreadCreateRetAddr));
    /**
     * Register this thread with the main thread
     */
    threadContextMap.pushBack(curContextPtr);

    void *threadFuncRetRlt = actualFuncPtr(argData);
    /**
     * Perform required actions after each thread function completes
     */
//    curContextPtr->endTImestamp = getunixtimestampms();
    saveData(curContextPtr);
    return threadFuncRetRlt;
}

extern bool installed;

// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    //register uint64_t rbp asm ("rbp");
    //register uint64_t rsp asm ("rsp");c
    //void **callerAddr1 = reinterpret_cast<void **>(rbp+8);
    //void *callerAddrPtr = *reinterpret_cast<void **>(rsp + 0x8 + 0x40);
    //uint8_t callOpCode = *reinterpret_cast<uint8_t *>((uint64_t) callerAddrPtr - 0x5);
    //if (callOpCode != 0xE8 && callOpCode != 0xFF && callOpCode != 0x9A) {
    //    fatalError("Failed to parse the caller address for pthread_create")
    //    exit(-1);
    //}
//    INFO_LOG("Pthread create");


    if (pthread_create_orig == nullptr) {
        //load plt hook address
        pthread_create_orig = (pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
        if (!pthread_create_orig) {
            fatalError("Cannot find the address of pthread_create");
            return false;
        }
    }

    if (!installed) {
        return pthread_create_orig(thread, attr, start, (void *) arg);
    }

    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
    args->actual_thread_function = start;
    args->data = arg;
    args->pthreadCreateRetAddr = __builtin_return_address(0);
    // Call the actual pthread_create

    int retVal = pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);

    HookContext *curContextPtr = curContext;
    //Attribute time to pthread_create

    return retVal;
}


int pthread_join(pthread_t __th, void **__thread_return) {
    if (pthread_join_orig == nullptr) {
        //load plt hook address
        pthread_join_orig = (pthread_join_origt) dlsym(RTLD_NEXT, "pthread_join");
        if (!pthread_join_orig) {
            fatalError("Cannot find the address of pthread_join");
            return 0;
        }
    }

    if (!installed) {
        return pthread_join_orig(__th, __thread_return);
    }
    HookContext *curContextPtr = curContext;


    uint64_t startTimestamp = getunixtimestampms();
    if (curContextPtr->indexPosi == 2) {
        //First layer. This is 2 because preHook has just been invoked
        curContextPtr->threadExecTime += (startTimestamp - curContextPtr->prevWallClockSnapshot) / threadNumPhase;
    }


    int retVal = pthread_join_orig(__th, __thread_return);
    uint64_t postHookClockCycles=getunixtimestampms();
    uint64_t clockCyclesDuration = postHookClockCycles - startTimestamp;

    scaler::SymID symbolId = curContextPtr->hookTuple[curContextPtr->indexPosi].symId;
    if (prevMaxThreadNumPhaseTimestamp > startTimestamp) {
        curContextPtr->recArr->internalArr[symbolId].totalClockCycles += clockCyclesDuration / prevMaxThreadNumPhase;
//        INFO_LOGS("PthreadJoin, prevMaxThreadNumPhase=%d",prevMaxThreadNumPhase);
    } else {
        curContextPtr->recArr->internalArr[symbolId].totalClockCycles += clockCyclesDuration / threadNumPhase;
//        INFO_LOGS("PthreadJoin, threadNumPhase=%d",threadNumPhase);
    }

    if (curContextPtr->indexPosi == 2) {
        //This is the first layer, this timestamp would be the next starting time of. This is 2 because postHookHander is not invoked yet
        curContextPtr->prevWallClockSnapshot = postHookClockCycles;
    }

    curContext->timeAlreadyAttributed = true; //Prevent postHook from overriding this time

    return retVal;
}
}