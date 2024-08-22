/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <cxxabi.h>
#include <type/RecTuple.h>
#include "util/tool/AtomicSpinLock.h"
#include <util/hook/LogicalClock.h>

namespace scaler {
    uint64_t logicalClock;
    std::atomic<uint64_t> wallclockSnapshot;
    uint64_t updateSpinlock;
    AtomicSpinLock threadUpdateLock; //Lock used in LogicalClock.h to update thread counters
    int32_t threadNum = 0; //Actual thread number recorded

    static thread_local DataSaver saverElem;

    HookContext *constructContext(scaler::ExtFuncCallHook &inst) {
        HookContext *rlt = new HookContext();
        if (!rlt) { fatalError("Cannot allocate memory for HookContext")
        }
        //INFO_LOGS("inst.curLoadingId.load(std::memory_order_acquire)+1=%zd",inst.curLoadingId.load(std::memory_order_acquire)+1);

        //Push a dummy value in the stack (Especially for callAddr, because we need to avoid null problem)
        rlt->hookTuple[rlt->indexPosi].callerAddr = 0;
        rlt->hookTuple[rlt->indexPosi].logicalClockCycles = 0;
        rlt->hookTuple[rlt->indexPosi].id.symId = 0;
        rlt->indexPosi = 1;

        rlt->dataSaved = false;

        assert(scaler::ExtFuncCallHook::instance != nullptr);
        rlt->_this = scaler::ExtFuncCallHook::instance;

        rlt->threadId = pthread_self();
        saverElem.initializeMe = 1; //Force initialize tls
        return rlt;
    }

    bool destructContext() {
        HookContext *curContextPtr = curContext;
        munmap(curContextPtr, sizeof(HookContext) +
                              sizeof(scaler::Array<uint64_t>) +
                              sizeof(pthread_mutex_t));
        curContext = nullptr;
        return true;
    }

    bool recOffsetVerified = false;

    inline void verifyRecOffset(HookContext * tmpContext) {
        if (!recOffsetVerified) {
            auto i  = (uint8_t *) tmpContext;
            auto j  = (uint8_t *) &tmpContext->recordingArray;
            auto k  = (uint8_t *) &tmpContext->recordingArray.internalArr;
            auto l  = (uint8_t *) &tmpContext->recordingArray.internalArr[0];
            auto m  = (uint8_t *) &tmpContext->recordingArray.internalArr[0].count;
            auto n  = (uint8_t *) &tmpContext->recordingArray.internalArr[0].gap;

            if (LDARR_OFFSET_IN_CONTEXT != j - i) { fatalErrorS("LDARR_OFFSET_IN_CONTEXT should be %zd", j - i);
            }

            if (INTERNALARR_OFFSET_IN_LDARR != k - j) { fatalErrorS("INTERNALARR_OFFSEjT_IN_LDARR should be %zd",
                                                                    k - j);
            }

            if (COUNT_OFFSET_IN_RECARR != m - l) { fatalErrorS("COUNT_OFFSET_IN_RECARR should be %zd", m - l);
            }

            if (GAP_OFFSET_IN_RECARR != n - l) { fatalErrorS("GAP_OFFSET_IN_RECARR should be %zd", n - l);
            }

            recOffsetVerified = true;
        }

    }


    void __attribute__((used, noinline, optimize(3))) accessTLS1() {
        HookContext *contextPtr  = curContext;
        auto &j  = contextPtr->recordingArray;
        auto &k  = contextPtr->recordingArray.internalArr;
        printf("%p.%p,%p", contextPtr, &j, k);


    }

    bool initTLS() {
        assert(scaler::ExtFuncCallHook::instance != nullptr);
        scaler::ExtFuncCallHook &inst = *scaler::ExtFuncCallHook::getInst();
        pthread_mutex_lock(&(inst.dynamicLoadingLock));

        //Put a dummy variable to avoid null checking
        //Initialize saving data structure
        HookContext *tempContext = constructContext(inst);

//#ifdef PRINT_DBG_LOG
        verifyRecOffset(tempContext);
//#endif

        if (!tempContext) { fatalError("Failed to allocate memory for Context");
            return false;
        }
        //Populate the recording array of existing loading ids

        populateRecordingArray(inst, tempContext);
        tempContext->initialized = SCALER_TRUE;

        curContext = tempContext;
        //INFO_LOGS("curContext=%p",curContext);
        pthread_mutex_unlock(&inst.dynamicLoadingLock);
        return true;
    }

    void populateRecordingArray(scaler::ExtFuncCallHook &inst, HookContext *curContextPtr) {
        //Do not need to acquire lock because lock has been acquired in
        //No contention because parent function will acquire a lock
        //Allocate recArray
        //INFO_LOGS("curContextPtr->recordingArray.allocate(%zd) %zd-%zd",inst.allExtSymbol.getSize()-curContextPtr->recordingArray.getSize(),inst.allExtSymbol.getSize(),curContextPtr->recordingArray.getSize());
        curContextPtr->recordingArray.allocate(inst.allExtSymbol.getSize() - curContextPtr->recordingArray.getSize());


        //Initialize gap to one
        for (ssize_t symId = 0; symId < inst.allExtSymbol.getSize(); ++symId) {
            //number mod 2^n is equivalent to stripping off all but the n lowest-order
            curContextPtr->recordingArray[symId].gap = inst.allExtSymbol[symId].initialGap;//0b11 if %4, because 4=2^2 Initially time everything
            curContextPtr->recordingArray[symId].count = 0;
        }

    }


    __thread HookContext *curContext __attribute((tls_model("initial-exec")));

    __thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE

    DataSaver::~DataSaver() {
        saveData(curContext);
    }

    inline void savePerThreadTimingData(std::stringstream &ss, HookContext *curContextPtr) {
        ss.str("");
        ss << scaler::ExtFuncCallHook::instance->folderName << "/threadTiming_" << curContextPtr->threadId << ".bin";
        //INFO_LOGS("Saving timing data to %s", ss.str().c_str());
        //INFO_LOGS("curContextPtr->recordingArray.getSize()=%zd",curContextPtr->recordingArray.getSize());
        int fd;
        size_t realFileIdSizeInBytes = sizeof(ThreadCreatorInfo) + sizeof(ArrayDescriptor) +
                                       curContextPtr->recordingArray.getSize() * sizeof(RecTuple);
        uint8_t *fileContentInMem = nullptr;
        if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) { fatalErrorS(
                    "Cannot fopen %s because:%s", ss.str().c_str(), strerror(errno));
        }
        uint8_t *_fileContentInMem = fileContentInMem;
        /**
         * Record who created the thread
         */
        ThreadCreatorInfo *threadCreatorInfo = reinterpret_cast<ThreadCreatorInfo *>(fileContentInMem);
        //Record thread name
        pthread_getname_np(pthread_self(),threadCreatorInfo->threadName,sizeof(threadCreatorInfo->threadName));
        threadCreatorInfo->threadFunctionPtr = curContextPtr->threadFunctionPtr;
        threadCreatorInfo->threadExecutionCycles = curContextPtr->threadExecTime;
        threadCreatorInfo->threadCreatorFileId = curContextPtr->threadCreatorFileId;
        threadCreatorInfo->magicNum = 167;
        fileContentInMem += sizeof(ThreadCreatorInfo);

        /**
         * Record size information about the recorded array
         */
        ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
        arrayDescriptor->arrayElemSize = sizeof(RecTuple);
        arrayDescriptor->arraySize = curContextPtr->recordingArray.getSize();
        arrayDescriptor->magicNum = 167;
        fileContentInMem += sizeof(ArrayDescriptor);

        /**
         * Write recording tuple onto the disk
         */
//    for(int i=0;i<curContextPtr->recordingArray.getSize();++i){
//        INFO_LOGS("SymbolId=%d Count=%zd Time=%zd",i,curContextPtr->recordingArray[i].count,curContextPtr->recordingArray[i].totalClockCycles);
//        if(curContextPtr->recordingArray[i].totalClockCycles>0){
//           INFO_LOGS("SymbolId=%d Count=%zd Time=%zd",i,curContextPtr->recordingArray[i].count,curContextPtr->recordingArray[i].totalClockCycles);
//        }
//    }
        memcpy(fileContentInMem, curContextPtr->recordingArray.data(),
               curContextPtr->recordingArray.getTypeSizeInBytes() * curContextPtr->recordingArray.getSize());

        if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) { fatalErrorS(
                    "Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
        }

        DBG_LOGS("Saving data to %s, %lu", scaler::ExtFuncCallHook::instance->folderName.c_str(), pthread_self());
    }

    inline void saveRealFileId(std::stringstream &ss, HookContext *curContextPtr) {
        ExtFuncCallHook *instance = ExtFuncCallHook::getInst();
        ss.str("");
        ss << instance->folderName << "/realFileId.bin";
        //The real id of each function is resolved in after hook, so I can only save it in datasaver

        int fd;
        ssize_t realFileIdSizeInBytes = sizeof(ArrayDescriptor) +
                                        (instance->allExtSymbol.getSize()) * sizeof(uint64_t);
        uint8_t *fileContentInMem = nullptr;
        if (!scaler::fOpen4Write<uint8_t>(ss.str().c_str(), fd, realFileIdSizeInBytes, fileContentInMem)) { fatalErrorS(
                    "Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }
        uint8_t *_fileContentInMem = fileContentInMem;

        /**
         * Write array descriptor first
         */
        ArrayDescriptor *arrayDescriptor = reinterpret_cast<ArrayDescriptor *>(fileContentInMem);
        arrayDescriptor->arrayElemSize = sizeof(uint64_t);
        arrayDescriptor->arraySize = instance->allExtSymbol.getSize();
        arrayDescriptor->magicNum = 167;
        fileContentInMem += sizeof(ArrayDescriptor);

        instance->parseRealFileId();
        uint64_t *realFileIdMem = reinterpret_cast<uint64_t *>(fileContentInMem);
        ssize_t allExtSymbolSize = instance->allExtSymbol.getSize();
        for (ssize_t i = 0; i < allExtSymbolSize; ++i) {
            realFileIdMem[i] = instance->allExtSymbol[i].realFileId;
        }

        if (!scaler::fClose<uint8_t>(fd, realFileIdSizeInBytes, _fileContentInMem)) { fatalErrorS(
                    "Cannot close file %s, because %s", ss.str().c_str(), strerror(errno));
        }
    }

    inline void saveDataForAllOtherThread(std::stringstream &ss, HookContext *curContextPtr) {
        DBG_LOG("Save data of all existing threads");
        for (int i = 0; i < threadContextMap.getSize(); ++i) {
            HookContext *threadContext = threadContextMap[i];
            saveData(threadContext);
        }
    }


    void saveData(HookContext *curContextPtr, bool finalize) {
        bypassCHooks = SCALER_TRUE;
        /* CS: Check whether data has been saved. Make sure data is only saved once */
        if (curContextPtr->dataSaved) {
            //INFO_LOGS("Thread data already saved, skip %d/%zd", i, threadContextMap.getSize());
            return;
        }
        curContextPtr->dataSaved = true;

        if (!curContextPtr->logicalClockSaved) {
            curContextPtr->logicalClockSaved = true;
            uint64_t curLogicalClock = threadTerminatedRecord();
            curContextPtr->threadExecTime = curLogicalClock - curContextPtr->threadExecTime;
        }


        /* CS: End */
        scaler::ExtFuncCallHook &inst = *scaler::ExtFuncCallHook::getInst();
        pthread_mutex_lock(&inst.dynamicLoadingLock);
        bool parsePMMapSuccess = inst.pmParser.parsePMMap();
        assert(parsePMMapSuccess);
        //INFO_LOGS("Thread %p %s has completed",pthread_self(),threadName);

        if (!curContext) { fatalError("curContext is not initialized, won't save anything");
            return;
        }

        //INFO_LOGS("AttributingThreadEndTime+= %lu - %lu", curLogicalClock, curContextPtr->threadExecTime);

        std::stringstream ss;

        savePerThreadTimingData(ss, curContextPtr);

        if (curContextPtr->isMainThread || finalize) {
            printf("Data saved to %s\n", scaler::ExtFuncCallHook::instance->folderName.c_str());
            saveRealFileId(ss, curContextPtr);
            saveDataForAllOtherThread(ss, curContextPtr);
        }

        pthread_mutex_unlock(&inst.dynamicLoadingLock);

    }

}


