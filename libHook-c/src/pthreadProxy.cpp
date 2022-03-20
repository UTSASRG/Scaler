#include <util/hook/proxy/pthreadProxy.h>

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
//    bypassCHooks = SCALER_TRUE;
//
//    /**
//     * Perform required actions at beginning of thread
//     */
//    initTLS();
//
//    /**
//     * Call actual thread function
//     */
//    // Extract arguments and call actual thread function
//    auto *args = static_cast<dummy_thread_function_args *>(data);
//    void *argData = args->data;
//    auto actualFuncPtr = args->actual_thread_function;
//    free(args);
//    args = nullptr;
//    bypassCHooks = SCALER_FALSE;
//
//    Context *curContextPtr = curContext;
//    if (scaler::libHookStartingAddr <= reinterpret_cast<uint64_t>(actualFuncPtr) &&
//        reinterpret_cast<uint64_t>(actualFuncPtr) <= scaler::libHookEndingAddr) {
//        //This thread is created by the hook itself, we don't save anything
//        DBG_LOGS("thread %lu is not created by myself", pthread_self());
//        isThreadCratedByMyself = true;
//    } else {
//        isThreadCratedByMyself = false;
//        curContextPtr->curThreadNumber += 1;
//        DBG_LOGS("thread %lu is created by myself", pthread_self());
//    }
//
//    //curContextPtr->threadCreationTimestamp = getunixtimestampms();
//    actualFuncPtr(argData);
//    /**
//     * Perform required actions after each thread function completes
//     */
//    curContextPtr->threadTerminatedPeacefully = true;
//    //curContextPtr->threadTerminateTimestamp = getunixtimestampms();

    return nullptr;
}

// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
//    bypassCHooks = SCALER_TRUE;
//    //register uint64_t rbp asm ("rbp");
//    //register uint64_t rsp asm ("rsp");
//    //void **callerAddr1 = reinterpret_cast<void **>(rbp+8);
//    //void *callerAddrPtr = *reinterpret_cast<void **>(rsp + 0x8 + 0x40);
//    //uint8_t callOpCode = *reinterpret_cast<uint8_t *>((uint64_t) callerAddrPtr - 0x5);
//    //if (callOpCode != 0xE8 && callOpCode != 0xFF && callOpCode != 0x9A) {
//    //    fatalError("Failed to parse the caller address for pthread_create")
//    //    exit(-1);
//    //}
//    DBG_LOGS("%lu created a thread pthread_create", pthread_self());
//
//    if (scaler::pthread_create_orig == nullptr) {
//        //load plt hook address
//        scaler::pthread_create_orig = (scaler::pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
//        if (!scaler::pthread_create_orig) {
//            fatalError("Cannot find the address of pthread_create");
//            return false;
//        }
//    }
//
//    auto threadID = pthread_self();
//    DBG_LOGS("pthread_create %lu", pthread_self());
//
//    if (scaler::ExtFuncCallHookAsm::getInst()->active()) {
//        // Prepare the inputs for the intermediate (dummy) thread function
//        auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
//        args->actual_thread_function = start;
//        args->data = arg;
//        bypassCHooks = SCALER_FALSE;
//        // Call the actual pthread_create
//        return scaler::pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);
//    } else {
//        //Asm hook not ready
//        bypassCHooks = SCALER_FALSE;
//        isThreadCratedByMyself = true;
//        return scaler::pthread_create_orig(thread, attr, start, (void *) arg);
//    }
    return 0;
}
}