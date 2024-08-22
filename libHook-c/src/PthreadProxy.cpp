/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <util/hook/proxy/PthreadProxy.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/hook/LogicalClock.h>

namespace scaler {

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

        //INFO_LOGS("Thread %p creates at logical clock %ld", pthread_self(), curContext->threadExecTime);

        /**
         * Call actual thread function
         */
        // Extract arguments and call actual thread function
        auto *args = static_cast<dummy_thread_function_args *>(data);
        void *argData = args->data;
        auto actualFuncPtr = args->actual_thread_function;
//        void *pthreadCreateRetAddr = args->pthreadCreateRetAddr;
        free(args);
        args = nullptr;

        HookContext *curContextPtr = curContext;
        assert(curContextPtr != NULL);
        curContextPtr->threadCreatorFileId = curContextPtr->_this->pmParser.findFileIdByAddr(
                (void *) (actualFuncPtr));
//    DBG_LOGS("Thread is created by %ld", curContextPtr->_this->pmParser.findFileIdByAddr(
//            pthreadCreateRetAddr));

        /**
         * Register this thread with the main thread
         */
        threadContextMap.pushBack(curContextPtr);

        /**
        * Update logical clock
        */
        uint64_t curLogicalClock = threadCreatedRecord(curContext->cachedWallClockSnapshot,
                                                       curContext->cachedLogicalClock,
                                                       curContext->cachedThreadNum);
        curContext->threadExecTime = curLogicalClock;
        void *threadFuncRetRlt = actualFuncPtr(argData);
        /**
         * Perform required actions after each thread function completes
         */
//    curContextPtr->endTImestamp = getunixtimestampms();
        saveData(curContextPtr);
        return threadFuncRetRlt;
    }

    extern bool installed;
    static void *libPthreadHandle = nullptr;

    typedef int (pthread_create_t)(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);

    static pthread_create_t *realPthreadCreateAddress = nullptr;

// Main Pthread wrapper functions.
    int pthread_create_proxy(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
        //register uint64_t rbp asm ("rbp");
        //register uint64_t rsp asm ("rsp");c
        //void **callerAddr1 = reinterpret_cast<void **>(rbp+8);
        //void *callerAddrPtr = *reinterpret_cast<void **>(rsp + 0x8 + 0x40);
        //uint8_t callOpCode = *reinterpret_cast<uint8_t *>((uint64_t) callerAddrPtr - 0x5);
        //if (callOpCode != 0xE8 && callOpCode != 0xFF && callOpCode != 0x9A) {
        //    fatalError("Failed to parse the caller address for pthread_create")
        //    exit(-1);
        //}
        //INFO_LOG("Pthread create");
        //todo: make sure this libary is a pre-eixsting library
        libPthreadHandle = dlopen("/lib/x86_64-linux-gnu/libpthread.so.0", RTLD_LAZY);
        assert(libPthreadHandle != nullptr);
        realPthreadCreateAddress = (pthread_create_t *) dlsym(libPthreadHandle, "pthread_create");
        assert(realPthreadCreateAddress != nullptr);
        if (!installed) {
            return realPthreadCreateAddress(thread, attr, start, (void *) arg);
        }

        //auto threadID = pthread_self();
        //INFO_LOGS("pthread_create %p", pthread_self());

        auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
        args->actual_thread_function = start;
        args->data = arg;
        // Call the actual pthread_create

        int retVal = realPthreadCreateAddress(thread, attr, dummy_thread_function, (void *) args);

        HookContext *curContextPtr = curContext;
        //Attribute time to pthread_create

        return retVal;
    }

//    int pthread_cond_timedwait_proxy(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime){
//        HookContext* curContextPtr=curContext;
//        //Active thread number -= 1
//        updateLogicalClockAndThreadNum(-1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        int result=pthread_cond_timedwait(cond,mutex,abstime);
//        updateLogicalClockAndThreadNum(+1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        return result;
//    }
//    int pthread_cond_wait_proxy(pthread_cond_t * cond, pthread_mutex_t * mutex){
//        HookContext* curContextPtr=curContext;
//        updateLogicalClockAndThreadNum(-1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        int result=pthread_cond_wait(cond,mutex);
//        updateLogicalClockAndThreadNum(+1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        return result;
//    }
//
//    int pthread_join_proxy(pthread_t thread, void **value_ptr){
//        HookContext* curContextPtr=curContext;
//        updateLogicalClockAndThreadNum(-1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        int result=pthread_join(thread,value_ptr);
//        updateLogicalClockAndThreadNum(+1,curContextPtr->cachedWallClockSnapshot,curContextPtr->cachedLogicalClock,curContextPtr->cachedThreadNum);
//        return result;
//    }
}