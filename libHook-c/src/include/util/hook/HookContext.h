/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef HOOK_CONTEXT_H
#define HOOK_CONTEXT_H

#include <util/datastructure/FStack.h>
#include <cstdio>
#include <util/tool/Timer.h>
#include <type/RecTuple.h>
#include <atomic>
#include "ExtFuncCallHook.h"

namespace scaler {

    extern int32_t threadNum; //The current number of threads
    extern std::atomic<uint64_t> wallclockSnapshot; //Used as update timestamp
    extern uint64_t logicalClock;

#define MAX_CALL_DEPTH 64 //N+1 because of dummy variable
#define MAX_DLOPEN_NUMBER 4096 //N+1 because of dummy variable

    union IdUnion {
        int64_t symId;
        int64_t fileId;
    };
    struct HookTuple {
        uint64_t callerAddr; //8
        uint64_t logicalClockCycles; //8
        IdUnion id; //8
    };

    struct HookContext {
        /**
         * Frequently read/write variables
         */
        uint64_t cachedWallClockSnapshot;  //8bytes
        uint64_t cachedLogicalClock; //8bytes
        uint32_t indexPosi;//4bytes
        uint32_t cachedThreadNum; //4bytes
        //Variables used to determine whether it's called by hook handler or not
        int64_t threadCreatorFileId = 1; //Which library created the current thread? The default one is main thread
        scaler::ExtFuncCallHook *_this = nullptr; //8bytes
        HookTuple hookTuple[MAX_CALL_DEPTH]; //8bytes aligned
        scaler::Array<RecTuple> recordingArray; //8bytes
        //Records which symbol is called for how many times, the index is scalerid (Only contains hooked function)

        /**
         * Infrequently read/write variables
         */
        void *threadFunctionPtr = nullptr;
        uint64_t threadExecTime; //Used for application time attribution
        pthread_t threadId;
        uint8_t dataSaved = false;
        uint8_t logicalClockSaved = false;
        uint8_t isMainThread = false;
        uint8_t initialized = 0;
    };

    const uint8_t SCALER_TRUE = 145;
    const uint8_t SCALER_FALSE = 167;
    const int LDARR_OFFSET_IN_CONTEXT = 0x628;
    const int INTERNALARR_OFFSET_IN_LDARR = 0x8;
    const int COUNT_OFFSET_IN_RECARR = 0x8;
    const int GAP_OFFSET_IN_RECARR = 0x10;


    class DataSaver {
    public:
        char initializeMe = 0;

        ~DataSaver();
    };


    void saveData(HookContext *context, bool finalize = false);


    extern __thread HookContext *curContext;

    extern __thread uint8_t bypassCHooks; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE

    extern scaler::Vector<HookContext *> threadContextMap;

    extern pthread_mutex_t threadDataSavingLock;

    bool initTLS();

    void populateRecordingArray(scaler::ExtFuncCallHook &inst,HookContext* curContextPtr);


}
#endif