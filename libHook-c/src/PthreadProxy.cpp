#include <util/hook/proxy/PthreadProxy.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/hook/LogicalClock.h>




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

    /**
     * Perform required actions at beginning of thread
     */
    initTLS();
    /**
    * Update logical clock
    */
    uint64_t curLogicalClock = threadCreatedRecord(curContext->cachedWallClockSnapshot, curContext->cachedLogicalClock,
                                                   curContext->cachedThreadNum);
    curContext->threadExecTime = curLogicalClock;
//    INFO_LOGS("Thread %ld creates at logical clock %ld", pthread_self(), curContext->threadExecTime);

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
//    DBG_LOGS("Thread is created by %ld", curContextPtr->_this->pmParser.findExecNameByAddr(
//            pthreadCreateRetAddr));

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
int pthread_create_proxy(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    if (!installed) {
        return pthread_create(thread, attr, start, (void *) arg);
    }

    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
    args->actual_thread_function = start;
    args->data = arg;
    args->pthreadCreateRetAddr = __builtin_return_address(0);
    // Call the actual pthread_create

    int retVal = pthread_create(thread, attr, dummy_thread_function, (void *) args);

    HookContext *curContextPtr = curContext;
    //Attribute time to pthread_create

    return retVal;
}


int pthread_join_proxy(pthread_t __th, void **__thread_return) {

    if (!installed) {
        return pthread_join(__th, __thread_return);
    }

    int retVal = pthread_join(__th, __thread_return);

    return retVal;
}
