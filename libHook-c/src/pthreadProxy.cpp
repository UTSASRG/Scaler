#include <util/hook/proxy/pthreadProxy.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>

extern "C" {
typedef int (*pthread_create_origt)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

typedef void (*pthread_exit_origt)(void *__retval);

typedef int (*pthread_cancel_origt)(pthread_t __th);

pthread_create_origt pthread_create_orig;


// Define structure to hold arguments for dummy thread function
struct dummy_thread_function_args {
    void *(*actual_thread_function)(void *data);

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
     * Call actual thread function
     */
    // Extract arguments and call actual thread function
    auto *args = static_cast<dummy_thread_function_args *>(data);
    void *argData = args->data;
    auto actualFuncPtr = args->actual_thread_function;
    free(args);
    args = nullptr;

    HookContext *curContextPtr = curContext;
    assert(curContextPtr != NULL);
    curContextPtr->curFileId = curContextPtr->_this->pmParser.findExecNameByAddr(
            (void *) actualFuncPtr);
    curContextPtr->startTImestamp = getunixtimestampms();
    actualFuncPtr(argData);
    /**
     * Perform required actions after each thread function completes
     */
    curContextPtr->endTImestamp = getunixtimestampms();

    return nullptr;
}

// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    //register uint64_t rbp asm ("rbp");
    //register uint64_t rsp asm ("rsp");
    //void **callerAddr1 = reinterpret_cast<void **>(rbp+8);
    //void *callerAddrPtr = *reinterpret_cast<void **>(rsp + 0x8 + 0x40);
    //uint8_t callOpCode = *reinterpret_cast<uint8_t *>((uint64_t) callerAddrPtr - 0x5);
    //if (callOpCode != 0xE8 && callOpCode != 0xFF && callOpCode != 0x9A) {
    //    fatalError("Failed to parse the caller address for pthread_create")
    //    exit(-1);
    //}
    ERR_LOG("Pthread create");


    if (pthread_create_orig == nullptr) {
        //load plt hook address
        pthread_create_orig = (pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
        if (!pthread_create_orig) {
            fatalError("Cannot find the address of pthread_create");
            return false;
        }
    }

    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
    args->actual_thread_function = start;
    args->data = arg;
    // Call the actual pthread_create
    return pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);
}
}