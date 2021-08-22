#ifdef __linux

#include <cstdint>
#include <sys/mman.h>
#include <algorithm>
#include <cassert>
#include <elf.h>
#include <set>
#include <immintrin.h>
#include <thread>
#include <map>
#include <utility>
#include <util/hook/ExtFuncCallHookBrkpoint.h>
#include <exceptions/ScalerException.h>
#include <util/tool/MemoryTool_Linux.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
#include <util/tool/Timer.h>
#include <type/InvocationTree.h>
#include <util/tool/PthreadParmExtractor.h>
#include <semaphore.h>
#include <util/tool/SemaphoreParmExtractor.h>

namespace scaler {

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::instance = nullptr;


    void ExtFuncCallHookBrkpoint::install(Hook::SYMBOL_FILTER filterCallB) {
        memTool = MemoryTool_Linux::getInst();
        pmParser.parsePMMap();

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //Step3: Use callback to determine which ID to hook
        std::set<size_t> fileToHook;

        for (auto iterFile = elfImgInfoMap.begin(); iterFile != elfImgInfoMap.end(); ++iterFile) {
            auto &curFileId = iterFile->first;
            auto &curFileName = pmParser.idFileMap.at(curFileId);
            auto &curELFImgInfo = iterFile->second;

            //loop through external symbols, let user decide which symbol to hook through callback function
            for (auto iterSymbol = curELFImgInfo.idFuncMap.begin();
                 iterSymbol != curELFImgInfo.idFuncMap.end(); ++iterSymbol) {
                auto &curSymbolId = iterSymbol->first;
                auto &curSymbolName = iterSymbol->second;
                if (filterCallB(curFileName, curSymbolName)) {
                    //The user wants this symbol
                    fileToHook.emplace(curFileId);

                    //If the function name matches common pthread functions. Store the function id in advance
                    if (curSymbolName == "pthread_create") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_CREATE = curSymbolId;
                    } else if (curSymbolName == "pthread_join") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_JOIN = curSymbolId;
                    } else if (curSymbolName == "pthread_tryjoin_np") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_TRYJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_timedjoin_np") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_TIMEDJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_clockjoin_np") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_CLOCKJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_lock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_LOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_timedlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_TIMEDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_clocklock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_CLOCKLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_unlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_rdlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_RDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_tryrdlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_timedrdlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_clockrdlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_wrlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_WRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_trywrlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_timedwrlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_clockwrlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_unlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_signal") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_SIGNAL = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_broadcast") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_BROADCAST = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_wait") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_WAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_timedwait") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_TIMEDWAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_clockwait") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_CLOCKWAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_lock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_LOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_trylock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_TRYLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_unlock") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_barrier_wait") {
                        curELFImgInfo.pthreadExtSymbolId.PTHREAD_BARRIER_WAIT = curSymbolId;
                    }

                    if (curSymbolName == "sem_wait") {
                        curELFImgInfo.semaphoreExtSymbolId.SEM_WAIT = curSymbolId;
                    } else if (curSymbolName == "sem_timedwait") {
                        curELFImgInfo.semaphoreExtSymbolId.SEM_TIMEDWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_clockwait") {
                        curELFImgInfo.semaphoreExtSymbolId.SEM_CLOCKWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_trywait") {
                        curELFImgInfo.semaphoreExtSymbolId.SEM_TRYWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_post") {
                        curELFImgInfo.semaphoreExtSymbolId.SEM_POST = curSymbolId;
                    }


                    auto &curSymbol = curELFImgInfo.allExtSymbol.at(curSymbolId);

                    //Step6: Insert breakpoint at .plt entry
                    //todo: we only use one of them. If ,plt.sec exists, hook .plt.sec rather than plt
                    recordOriCode(curSymbol.extSymbolId, curSymbol.pltEntry, true);

                    recordOriCode(curSymbol.extSymbolId, curSymbol.pltSecEntry, true);

                    //todo: Add logic to determine whether hook .plt or .plt.sec. Currently only hook .plt.sec
                    assert(pmParser.fileBaseAddrMap.at(curFileId).first <= curSymbol.pltSecEntry &&
                           curSymbol.pltSecEntry < pmParser.fileBaseAddrMap.at(curFileId).second);
                    DBG_LOGS("Instrumented pltsec code for symbol: %s:%s at:%p",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             curSymbol.pltSecEntry);
                    insertBrkpointAt(curSymbol.pltSecEntry, childMainThreadTID);

                    curELFImgInfo.hookedExtSymbol[curSymbol.extSymbolId] = curSymbol;
                }
            }
        }
    }

    thread_local SerilizableInvocationTree invocationTreeBrkpoint;
    thread_local InvocationTreeNode *curNodeBrkpoint = &invocationTreeBrkpoint.treeRoot;

    ExtFuncCallHookBrkpoint::ExtFuncCallHookBrkpoint(pid_t childPID)
            : pmParser(childPID), ExtFuncCallHook_Linux(pmParser, *MemoryTool_Linux::getInst()) {
        this->childMainThreadTID = childPID;
        invocationTreeBrkpoint.libPltHook = this;

    }

    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::getInst(pid_t childPID) {
        if (!instance)
            instance = new ExtFuncCallHookBrkpoint(childPID);
        return instance;
    }

    void ExtFuncCallHookBrkpoint::uninstall() {
    }


    ExtFuncCallHookBrkpoint::~ExtFuncCallHookBrkpoint() {

    }

    void ExtFuncCallHookBrkpoint::parseRelaSymbol(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo, size_t curFileID) {
        //Copy memory from other process to this process. We don't need to worry about the original pointer, because those memory are allocated by the OS.
        //ELF file segments will also be automatically freed after ELFParser is deconstructed.

        //1.copy .plt.rela contents to current process
        curELFImgInfo.relaPlt = static_cast<ElfW(Rela) *>(pmParser.readProcMem(curELFImgInfo.relaPlt,
                                                                               curELFImgInfo.relaPltCnt *
                                                                               sizeof(ElfW(Rela))));

        size_t maxDynSymId = findDynSymTblSize(curELFImgInfo);

        //2.copy dynamic string table to current process
        curELFImgInfo.dynSymTable = reinterpret_cast<const Elf64_Sym *>(pmParser.readProcMem(
                (void *) curELFImgInfo.dynSymTable,
                (maxDynSymId + 1) *
                sizeof(ElfW(Sym))));

        curELFImgInfo.dynStrTable = static_cast<const char *>(pmParser.readProcMem((void *) curELFImgInfo.dynStrTable,
                                                                                   curELFImgInfo.dynStrSize));

        //Copy other process memory to this memory
        ExtFuncCallHook_Linux::parseRelaSymbol(curELFImgInfo, curFileID);

        //Deallocate memory
        free(curELFImgInfo.relaPlt);
        free((void *) curELFImgInfo.dynSymTable);
        free((void *) curELFImgInfo.dynStrTable);
    }

    size_t ExtFuncCallHookBrkpoint::findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo) {
        ssize_t maxID = -1;
        //Since we don't know the size of .dynsym table, we have to use relocation table to figure it out.
        for (size_t i = 0; i < curELFImgInfo.relaPltCnt; ++i) {
            ElfW(Rela) *curRelaPlt = curELFImgInfo.relaPlt + i;
            ssize_t idx = ELFW(R_SYM)(curRelaPlt->r_info);
            if (idx > maxID)
                maxID = idx;
        }
        return maxID;

    }


    void ExtFuncCallHookBrkpoint::insertBrkpointAt(void *addr, int childTid) {
        unsigned long oriCode = *brkPointInfo.brkpointCodeMap.at(addr);

    }


    class ContextBrkpoint {
    public:
        //todo: Initialize using maximum stack size
        std::vector<size_t> extSymbolId;
        std::vector<size_t> fileId;
        //Variables used to determine whether it's called by hook handler or not
        std::vector<void *> callerAddr;
        std::vector<int64_t> timestamp;
        std::vector<pthread_t *> pthreadIdPtr;

        ContextBrkpoint();

        ContextBrkpoint(ContextBrkpoint &);

        ContextBrkpoint(ContextBrkpoint &&) noexcept;

        ContextBrkpoint &operator=(ContextBrkpoint &other);

    };

    ContextBrkpoint::ContextBrkpoint(ContextBrkpoint &rho) {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
    }

    ContextBrkpoint::ContextBrkpoint(ContextBrkpoint &&rho) noexcept {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
    }

    ContextBrkpoint &ContextBrkpoint::operator=(ContextBrkpoint &rho) {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
        return *this;
    }

    ContextBrkpoint::ContextBrkpoint() {

    }

    std::map<unsigned long long, ContextBrkpoint> brkpointCurContext;

    void
    ExtFuncCallHookBrkpoint::preHookHandler(size_t curFileID, size_t extSymbolId, void *callerAddr, void *brkpointLoc,
                                            int childTid) {
        auto startTimeStamp = getunixtimestampms();
        auto &curContext = brkpointCurContext[childTid];

        curContext.timestamp.push_back(getunixtimestampms());
        curContext.callerAddr.push_back(callerAddr);
        curContext.fileId.push_back(curFileID);
        curContext.extSymbolId.push_back(extSymbolId);


        //ELFImgInfo &curELFImgInfo = elfImgInfoMap.at(curFileID);

//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

//        DBG_LOGS("[Prehook %d] %s in %s is called in %s", childTid, curELFImgInfo.idFuncMap.at(extSymbolId).c_str(),
//                 "unknownLib",
//                 curELFImgInfo.filePath.c_str());

        //Check if a breakpoint is inserted at return address
        if (!brkPointInstalledAt(callerAddr)) {
            //Breakpoint not installed
            //DBG_LOGS("[Prehook %d] Afterhook breakpoint not installed for %s, install now", childTid,
            //         curELFImgInfo.idFuncMap.at(curFuncID).c_str());
            //Mark it as installed
            recordOriCode(extSymbolId, callerAddr);
            insertBrkpointAt(callerAddr, childTid);
        }


    }

    void
    ExtFuncCallHookBrkpoint::afterHookHandler(int childTid) {


        auto &curContext = brkpointCurContext[childTid];

//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

        if (curContext.fileId.size() <= 0) {
            return;
        }

        int64_t startTimestamp = curContext.timestamp.at(curContext.timestamp.size() - 1);
        curContext.timestamp.pop_back();

        void *callerAddr = curContext.callerAddr.at(curContext.callerAddr.size() - 1);
        curContext.callerAddr.pop_back();

        size_t fileId = curContext.fileId.at(curContext.fileId.size() - 1);
        curContext.fileId.pop_back();

        ELFImgInfo &curELFImgInfo = elfImgInfoMap.at(fileId);

        size_t funcId = curContext.extSymbolId.at(curContext.extSymbolId.size() - 1);
        curContext.extSymbolId.pop_back();
        auto &funcName = curELFImgInfo.idFuncMap.at(funcId);

        //if (curELFImgInfo.hookedExtSymbol.find(funcId) == curELFImgInfo.hookedExtSymbol.end()) {
        //    DBG_LOGS("%d %lu", curELFImgInfo.hookedExtSymbol.find(0) == curELFImgInfo.hookedExtSymbol.end(),
        //             curELFImgInfo.hookedExtSymbol.size());
        //   for (auto iter = curELFImgInfo.hookedExtSymbol.begin();
        //        iter != curELFImgInfo.hookedExtSymbol.end(); ++iter) {
        //       DBG_LOGS("%ld:%s", iter->first, iter->second.symbolName.c_str());
        //   }
        //   assert(false);
        //}

        int64_t endTimestamp = getunixtimestampms();

        auto &curSymbol = curELFImgInfo.hookedExtSymbol.at(funcId);
        if (curSymbol.addr == nullptr) {
            //Fill addr with the resolved address from GOT
            void **remoteData = static_cast<void **>(pmParser.readProcMem(curSymbol.gotEntry, sizeof(void *)));
            curSymbol.addr = *remoteData;
            free(remoteData);
        }
        auto libraryFileId = pmParser.findExecNameByAddr(curSymbol.addr);
        auto &libraryFileName = pmParser.idFileMap.at(libraryFileId);

        curSymbol.libraryFileID = libraryFileId;
        curNodeBrkpoint->setRealFileID(libraryFileId);
        curNodeBrkpoint->setFuncAddr(reinterpret_cast<int64_t>(curSymbol.addr));
        curNodeBrkpoint->setEndTimestamp(endTimestamp);
        curNodeBrkpoint = curNodeBrkpoint->getParent();


        DBG_LOGS("[Afterhook %d] %s in %s is called in %s", childTid, curSymbol.symbolName.c_str(),
                 libraryFileName.c_str(),
                 curELFImgInfo.filePath.c_str());

        if (funcId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_CREATE) {
            //todo: A better way is to compare function id rather than name. This is more efficient.
            //todo: A better way is to also compare library id because a custom library will also implement pthread_create.
            pthread_t *pthreadIdPtr = curContext.pthreadIdPtr.at(curContext.pthreadIdPtr.size() - 1);
            pmParser.readProcMem(pthreadIdPtr, (size_t) pthreadIdPtr);

            DBG_LOGS("[After Hook Param Parser]    pthread_create tid=%lu", *pthreadIdPtr);
        }


    }


    bool
    ExtFuncCallHookBrkpoint::parseSymbolInfo(size_t &curFileID, size_t &curFuncID, void *&callerAddr,
                                             void *&brkpointLoc,
                                             int childTid) {
//        //Parse symbol info based on rip, rsp
//        //todo:only need rip
//        if (ptrace(PTRACE_GETREGS, childTid, 0, &regs) < 0) {
//            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_POKETEXT failed because: %s", strerror(errno));
//        }
//
//
//        //todo: we could also intercept a later instruction and parse function id from stack
//        //todo: We could also map addr directly to both func and file id
//        brkpointLoc = reinterpret_cast<void *>(regs.rip - 1);
//
//        curFileID = pmParser.findExecNameByAddr(brkpointLoc);
//
//        if (brkPointInfo.brkpointFuncMap.find(brkpointLoc) == brkPointInfo.brkpointFuncMap.end()) {
//            ERR_LOGS("Unexpected stop because rip=%p doesn't seem to be caused by hook", brkpointLoc);
//            return false;
//        }
//
//        //DBG_LOGS("RIP=%p RSP=%p TID=%d\n", regs.rip, regs.rsp, childTid);
//        void **realCallAddr = (void **) pmParser.readProcMem(reinterpret_cast<void *>(regs.rsp), sizeof(void *));
//        callerAddr = *realCallAddr;
//        free(realCallAddr);
//
//        //DBG_LOGS("Child stopped at RIP = %p offset=%llu", regs.rip,
//        //         regs.rip - (uint64_t) elfImgInfoMap.at(curFileID).baseAddr);
//        //DBG_LOGS("Calleraddr= %p offset=%llu", regs.rsp,
//        //         regs.rsp - (uint64_t) elfImgInfoMap.at(curFileID).baseAddr);
//
//        curFuncID = brkPointInfo.brkpointFuncMap.at(brkpointLoc);
        return true;

    }

    bool ExtFuncCallHookBrkpoint::brkPointInstalledAt(void *addr) {
        //If a breakpoint is installed, then it must be in pltCodeInfoMap::addrFuncMap
        auto &curBrkpointCodeMap = brkPointInfo.brkpointCodeMap;
        return curBrkpointCodeMap.find(addr) != curBrkpointCodeMap.end();
    }

    void ExtFuncCallHookBrkpoint::recordOriCode(const size_t &funcID, void *addr, bool isPLT) {
        //Get the plt data of curSymbol
        //todo: .plt size is hard coded
        //Warning: this memory should be freed in ~PltCodeInfo

        brkPointInfo.brkpointCodeMap[addr] = static_cast<unsigned long *>(pmParser.readProcMem(addr,
                                                                                               sizeof(unsigned long)));
        brkPointInfo.brkpointFuncMap[addr] = funcID;

        if (isPLT)
            brkPointInfo.brkpointPltAddr.insert(addr);
    }

    bool ExtFuncCallHookBrkpoint::isBrkPointLocPlt(void *brkpointLoc) {
        auto &curBreakpointPLTAddr = brkPointInfo.brkpointPltAddr;
        return curBreakpointPLTAddr.find(brkpointLoc) != curBreakpointPLTAddr.end();
    }

    void ExtFuncCallHookBrkpoint::skipBrkPoint(void *brkpointLoc, int childTid,
                                               bool continueAfterSkip) {

    }

    void ExtFuncCallHookBrkpoint::brkpointEmitted(int childTid) {
//        size_t curFileID = 0;
//        size_t curFuncID = 0;
//        void *callerAddr = nullptr;
//        void *oriBrkpointLoc = nullptr;
//        struct user_regs_struct regs{};
//        if (!parseSymbolInfo(curFileID, curFuncID, callerAddr, oriBrkpointLoc, regs, childTid)) {
//            //Brakpoint failed, may cause by sigstop sent by other process, continue.
//            //todo: Maybe I should pass signal to target instead
//            if (ptrace(PTRACE_CONT, childTid, 0, 0) < 0) {
//                throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
//            }
//            return;
//        }
//
//        //Check if the breakpoint loc is a plt address
//        if (isBrkPointLocPlt(oriBrkpointLoc)) {
//            preHookHandler(curFileID, curFuncID, callerAddr, oriBrkpointLoc, regs, childTid);
//        } else {
//            afterHookHandler(childTid);
//        }
//        skipBrkPoint(oriBrkpointLoc, regs, childTid);
    }


    void ExtFuncCallHookBrkpoint::parseFuncInfo(size_t callerFileID, int64_t fileIDInCaller, void *&funcAddr,
                                                int64_t &libraryFileID) {
        //Find correct symbol
        auto &curSymbol = elfImgInfoMap.at(callerFileID).hookedExtSymbol.at(fileIDInCaller);

        if (curSymbol.symbolName == "exit") {
            int j = 1;
        }

        void **symbolAddr = (void **) pmParser.readProcMem(curSymbol.gotEntry, sizeof(curSymbol.gotEntry));

        //Parse address from got table
        curSymbol.addr = *symbolAddr;
        funcAddr = curSymbol.addr;
        //Search the fileID
        libraryFileID = pmParser.findExecNameByAddr(curSymbol.addr);
        assert(libraryFileID != -1);
        curSymbol.libraryFileID = libraryFileID;
    }


    ExtFuncCallHookBrkpoint::BrkPointInfo::~BrkPointInfo() {
        //Free plt code
        for (auto iter = brkpointCodeMap.begin(); iter != brkpointCodeMap.end(); ++iter) {
            if (iter->second != nullptr) {
                free(iter->second);
                iter->second = nullptr;
            }
        }
    }
}


#endif
