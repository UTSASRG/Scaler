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
#include <util/hook/ExtFuncCallHookPtrace.h>
#include <exceptions/ScalerException.h>
#include <util/tool/MemToolPtrace.h>
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
    ExtFuncCallHookPtrace *ExtFuncCallHookPtrace::instance = nullptr;


    void ExtFuncCallHookPtrace::install(Hook::SYMBOL_FILTER filterCallB) {
        //Add mian thread tid to the list, mainthread tid=pid
        tracedTID.insert(childMainThreadTID);


        int wait_status;
        //Wait for the child to stop before execution so we can insert breakpoints
        //This is where the child is first loaded into memory
        if (waitpid(childMainThreadTID, &wait_status, 0) < 0) {
            throwScalerExceptionS(ErrCode::WAIT_FAIL, "WAIT_FAIL failed because: %s", strerror(errno));
        }

        //Also trace newly created threads
        if (ptrace(PTRACE_SETOPTIONS, childMainThreadTID, 0, PTRACE_O_TRACECLONE | PTRACE_O_EXITKILL) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_SETOPTIONS failed because: %s", strerror(errno));
        }


        if (!WIFSTOPPED(wait_status)) {
            throwScalerExceptionS(ErrCode::WAIT_FAIL, "Child emit signal other than SIG_STOP, but is %s",
                                  strsignal(WSTOPSIG(wait_status)));
        }

        memTool = MemoryToolPtrace::getInst(pmParser);

        waitBeforeMainExecution();

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



        /* The child can continue running now */
        if (ptrace(PTRACE_CONT, childMainThreadTID, 0, 0) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
        }

        debuggerLoop();
    }

    thread_local SerilizableInvocationTree invocationTreePtrace;
    thread_local InvocationTreeNode *curNodePtrace = &invocationTreePtrace.treeRoot;

    ExtFuncCallHookPtrace::ExtFuncCallHookPtrace(pid_t childPID)
            : pmParser(childPID), ExtFuncCallHook_Linux(pmParser, *MemoryToolPtrace::getInst(pmParser)) {
        this->childMainThreadTID = childPID;
        invocationTreePtrace.libPltHook = this;

    }

    ExtFuncCallHookPtrace *ExtFuncCallHookPtrace::getInst(pid_t childPID) {
        if (!instance)
            instance = new ExtFuncCallHookPtrace(childPID);
        return instance;
    }

    void ExtFuncCallHookPtrace::uninstall() {
    }


    ExtFuncCallHookPtrace::~ExtFuncCallHookPtrace() {

    }

    void ExtFuncCallHookPtrace::parseRelaSymbol(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo, size_t curFileID) {
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

    size_t ExtFuncCallHookPtrace::findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo) {
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


    void ExtFuncCallHookPtrace::insertBrkpointAt(void *addr, int childTid) {
        unsigned long oriCode = *brkPointInfo.brkpointCodeMap.at(addr);

        //unsigned long orig_data = ptrace(PTRACE_PEEKTEXT, childPID, addr, 0);
        //assert(oriCode==orig_data);

        ptrace(PTRACE_POKETEXT, childTid, addr, (oriCode & ~(0xFF)) | 0xCC);
    }


    void ExtFuncCallHookPtrace::debuggerLoop() {
        DBG_LOG("debugger started");


        int waitStatus;
        while (true) {
            //DBG_LOGS("Wait for child", strsignal(WSTOPSIG(wait_status)));
            int childTid = waitForAllChild(waitStatus);

            if (childTid == -1) {
                DBG_LOG("No thread left, exit");
                break;
            }

            if (waitStatus >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) {
                //DBG_LOGS("Child %d created a new thread", childTid);
                newThreadCreated(childTid);
            } else if (WIFSTOPPED(waitStatus)) {
                // DBG_LOGS("Child %d got stop signal: %s", childTid, strsignal(WSTOPSIG(waitStatus)));
                brkpointEmitted(childTid);
            } else if (WIFEXITED(waitStatus)) {
                DBG_LOGS("Child %d exited. remove it from list, exitStatus %d", childTid, WEXITSTATUS(waitStatus));
                threadExited(childTid);
            } else {
                throwScalerException(ErrCode::UNKNOWN_SIGNAL, "Unexpected signal");
            }
        }
    }

    class Context {
    public:
        //todo: Initialize using maximum stack size
        std::vector<size_t> extSymbolId;
        std::vector<size_t> fileId;
        //Variables used to determine whether it's called by hook handler or not
        std::vector<void *> callerAddr;
        std::vector<int64_t> timestamp;
        std::vector<pthread_t *> pthreadIdPtr;

        Context();

        Context(Context &);

        Context(Context &&) noexcept;

        Context &operator=(Context &other);

    };

    Context::Context(Context &rho) {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
    }

    Context::Context(Context &&rho) noexcept {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
    }

    Context &Context::operator=(Context &rho) {
        extSymbolId = rho.extSymbolId;
        fileId = rho.fileId;
        callerAddr = rho.callerAddr;
        timestamp = rho.timestamp;
        return *this;
    }

    Context::Context() {

    }

    std::map<unsigned long long, Context> ptraceCurContext;

    void
    ExtFuncCallHookPtrace::preHookHandler(size_t curFileID, size_t extSymbolId, void *callerAddr, void *brkpointLoc,
                                          user_regs_struct &regs, int childTid) {
        auto startTimeStamp = getunixtimestampms();
        auto &curContext = ptraceCurContext[childTid];

        curContext.timestamp.push_back(getunixtimestampms());
        curContext.callerAddr.push_back(callerAddr);
        curContext.fileId.push_back(curFileID);
        curContext.extSymbolId.push_back(extSymbolId);


        ELFImgInfo &curELFImgInfo = elfImgInfoMap.at(curFileID);

//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

        DBG_LOGS("[Prehook %d] %s in %s is called in %s", childTid, curELFImgInfo.idFuncMap.at(extSymbolId).c_str(),
                 "unknownLib",
                 curELFImgInfo.filePath.c_str());

        //Check if a breakpoint is inserted at return address
        if (!brkPointInstalledAt(callerAddr)) {
            //Breakpoint not installed
            //DBG_LOGS("[Prehook %d] Afterhook breakpoint not installed for %s, install now", childTid,
            //         curELFImgInfo.idFuncMap.at(curFuncID).c_str());
            //Mark it as installed
            recordOriCode(extSymbolId, callerAddr);
            insertBrkpointAt(callerAddr, childTid);
        }

        if (curELFImgInfo.pthreadExtSymbolId.isFuncPthread(extSymbolId)) {
            //Add a tree node
            auto *newNode = new PthreadInvocationTreeNode();
            newNode->setExtFuncID(extSymbolId);
            newNode->setStartTimestamp(startTimeStamp);
            //Insert at back
            curNodePtrace = curNodePtrace->addChild(newNode);

            if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_CREATE) {
                //todo: A better way is to compare function id rather than name. This is more efficient.
                //todo: A better way is to also compare library id because a custom library will also implement pthread_create.
                pthread_t *newThread;
                parm_pthread_create(newThread, pmParser, regs.rdi);
                curContext.pthreadIdPtr.emplace_back(newThread);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_JOIN) {
                pthread_t joinThread;
                parm_pthread_join(joinThread, pmParser, regs.rdi);
                newNode->extraField1 = joinThread;
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_join tid=%lu", childTid,
                         joinThread);

            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_TRYJOIN_NP) {
                pthread_t joinThread;
                parm_pthread_tryjoin_np(joinThread, pmParser, regs.rdi);
                newNode->extraField1 = joinThread;
                DBG_LOGS("[Pre Hook Param Parser]   callingthread=%d pthread_tryjoin_np tid=%lu", childTid,
                         joinThread);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_TIMEDJOIN_NP) {
                pthread_t joinThread;
                parm_pthread_timedjoin_np(joinThread, pmParser, regs.rdi);
                newNode->extraField1 = joinThread;
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_timedjoin_np tid=%lu", childTid,
                         joinThread);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_CLOCKJOIN_NP) {
                pthread_t joinThread;
                parm_pthread_clockjoin_np(joinThread, pmParser, regs.rdi);
                newNode->extraField1 = joinThread;
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_clockjoin_np tid=%lu", childTid,
                         joinThread);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_LOCK) {
                pthread_mutex_t *mutex_t;
                parm_pthread_mutex_lock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_mutex_lock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_TIMEDLOCK) {
                pthread_mutex_t *mutex_t;
                parm_pthread_mutex_timedlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_mutex_timedlock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_CLOCKLOCK) {
                pthread_mutex_t *mutex_t;
                parm_pthread_mutex_clocklock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_mutex_clocklock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_UNLOCK) {
                pthread_mutex_t *mutex_t;
                parm_pthread_mutex_unlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  parm_pthread_mutex_unlock lID=%p",
                         childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_RDLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_rdlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_rdlock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYRDLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_tryrdlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_tryrdlock lID=%p",
                         childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDRDLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_timedrdlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_timedrdlock lID=%p",
                         childTid, mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKRDLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_clockrdlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_clockrdlock lID=%p",
                         childTid, mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_WRLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_wrlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_wrlock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYWRLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_trywrlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_trywrlock lID=%p",
                         childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDWRLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_timedwrlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_timedwrlock lID=%p",
                         childTid, mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKWRLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_clockwrlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_clockwrlock lID=%p",
                         childTid, mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_UNLOCK) {
                pthread_rwlock_t *mutex_t;
                parm_pthread_rwlock_unlock(mutex_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_rwlock_unlock lID=%p", childTid,
                         mutex_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_SIGNAL) {
                pthread_cond_t *cond_t;
                parm_pthread_cond_signal(cond_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(cond_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_cond_signal condID=%p", childTid,
                         cond_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_BROADCAST) {
                pthread_cond_t *cond_t;
                parm_pthread_cond_broadcast(cond_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(cond_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_cond_broadcast condID=%p",
                         childTid,
                         cond_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_WAIT) {
                pthread_cond_t *cond_t;
                pthread_mutex_t *mutex_t;
                parm_pthread_cond_wait(cond_t, mutex_t, pmParser, regs.rdi, regs.rsi);
                newNode->extraField1 = reinterpret_cast<int64_t>(cond_t);
                newNode->extraField2 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_cond_wait condID=%p", childTid,
                         cond_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_TIMEDWAIT) {
                pthread_cond_t *cond_t;
                pthread_mutex_t *mutex_t;
                parm_pthread_cond_timedwait(cond_t, mutex_t, pmParser, regs.rdi, regs.rsi);
                newNode->extraField1 = reinterpret_cast<int64_t>(cond_t);
                newNode->extraField2 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_cond_timedwait condID=%p",
                         childTid,
                         cond_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_COND_CLOCKWAIT) {
                pthread_cond_t *cond_t;
                pthread_mutex_t *mutex_t;
                parm_pthread_cond_clockwait(cond_t, mutex_t, pmParser, regs.rdi, regs.rsi);
                newNode->extraField1 = reinterpret_cast<int64_t>(cond_t);
                newNode->extraField2 = reinterpret_cast<int64_t>(mutex_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_cond_clockwait condId=%p",
                         childTid,
                         cond_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_LOCK) {
                pthread_spinlock_t *spinlock_t;
                parm_pthread_spin_lock(spinlock_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(spinlock_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_spin_lock lID=%p", childTid,
                         spinlock_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_TRYLOCK) {
                pthread_spinlock_t *spinlock_t;
                parm_pthread_spin_trylock(spinlock_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(spinlock_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_spin_trylock lID=%p", childTid,
                         spinlock_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_UNLOCK) {
                pthread_spinlock_t *spinlock_t;
                parm_pthread_spin_unlock(spinlock_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(spinlock_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_spin_unlock lID=%p", childTid,
                         spinlock_t);
            } else if (extSymbolId == curELFImgInfo.pthreadExtSymbolId.PTHREAD_BARRIER_WAIT) {
                pthread_barrier_t *barrier_t;
                parm_pthread_barrier_wait(barrier_t, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(barrier_t);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  pthread_barrier_wait barrierId=%p",
                         childTid, barrier_t);
            }
        } else if (curELFImgInfo.semaphoreExtSymbolId.isFuncSemaphore(extSymbolId)) {
            //Add a tree node
            auto *newNode = new SemaphoreInvocationTreeNode();
            newNode->setExtFuncID(extSymbolId);
            newNode->setStartTimestamp(startTimeStamp);
            //Insert at back
            curNodePtrace = curNodePtrace->addChild(newNode);

            if (extSymbolId == curELFImgInfo.semaphoreExtSymbolId.SEM_WAIT) {
                sem_t *__sem;
                parm_sem_wait(__sem, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(__sem);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  sem_wait sID=%p", childTid, __sem);
            } else if (extSymbolId == curELFImgInfo.semaphoreExtSymbolId.SEM_TIMEDWAIT) {
                sem_t *__sem;
                parm_sem_timedwait(__sem, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(__sem);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  sem_timedwait sID=%p", childTid, __sem);
            } else if (extSymbolId == curELFImgInfo.semaphoreExtSymbolId.SEM_CLOCKWAIT) {
                sem_t *__sem;
                parm_sem_clockwait(__sem, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(__sem);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  sem_clockwait sID=%p", childTid, __sem);
            } else if (extSymbolId == curELFImgInfo.semaphoreExtSymbolId.SEM_TRYWAIT) {
                sem_t *__sem;
                parm_sem_trywait(__sem, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(__sem);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  sem_trywait sID=%p", childTid, __sem);
            } else if (extSymbolId == curELFImgInfo.semaphoreExtSymbolId.SEM_POST) {
                sem_t *__sem;
                parm_sem_post(__sem, pmParser, regs.rdi);
                newNode->extraField1 = reinterpret_cast<int64_t>(__sem);
                DBG_LOGS("[Pre Hook Param Parser]    callingthread=%d  sem_post sID=%p", childTid, __sem);
            }

        } else {
            //Add a tree node
            auto *newNode = new InvocationTreeNode();
            newNode->setExtFuncID(extSymbolId);
            newNode->setStartTimestamp(startTimeStamp);
            //Insert at back
            curNodePtrace = curNodePtrace->addChild(newNode);
        }

    }

    void
    ExtFuncCallHookPtrace::afterHookHandler(int childTid) {


        auto &curContext = ptraceCurContext[childTid];

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
        curNodePtrace->setRealFileID(libraryFileId);
        curNodePtrace->setFuncAddr(reinterpret_cast<int64_t>(curSymbol.addr));
        curNodePtrace->setEndTimestamp(endTimestamp);
        curNodePtrace = curNodePtrace->getParent();


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
    ExtFuncCallHookPtrace::parseSymbolInfo(size_t &curFileID, size_t &curFuncID, void *&callerAddr, void *&brkpointLoc,
                                           user_regs_struct &regs, int childTid) {
        //Parse symbol info based on rip, rsp
        //todo:only need rip
        if (ptrace(PTRACE_GETREGS, childTid, 0, &regs) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_POKETEXT failed because: %s", strerror(errno));
        }


        //todo: we could also intercept a later instruction and parse function id from stack
        //todo: We could also map addr directly to both func and file id
        brkpointLoc = reinterpret_cast<void *>(regs.rip - 1);

        curFileID = pmParser.findExecNameByAddr(brkpointLoc);

        if (brkPointInfo.brkpointFuncMap.find(brkpointLoc) == brkPointInfo.brkpointFuncMap.end()) {
            ERR_LOGS("Unexpected stop because rip=%p doesn't seem to be caused by hook", brkpointLoc);
            return false;
        }

        //DBG_LOGS("RIP=%p RSP=%p TID=%d\n", regs.rip, regs.rsp, childTid);
        void **realCallAddr = (void **) pmParser.readProcMem(reinterpret_cast<void *>(regs.rsp), sizeof(void *));
        callerAddr = *realCallAddr;
        free(realCallAddr);

        //DBG_LOGS("Child stopped at RIP = %p offset=%llu", regs.rip,
        //         regs.rip - (uint64_t) elfImgInfoMap.at(curFileID).baseAddr);
        //DBG_LOGS("Calleraddr= %p offset=%llu", regs.rsp,
        //         regs.rsp - (uint64_t) elfImgInfoMap.at(curFileID).baseAddr);

        curFuncID = brkPointInfo.brkpointFuncMap.at(brkpointLoc);
        return true;

    }

    bool ExtFuncCallHookPtrace::brkPointInstalledAt(void *addr) {
        //If a breakpoint is installed, then it must be in pltCodeInfoMap::addrFuncMap
        auto &curBrkpointCodeMap = brkPointInfo.brkpointCodeMap;
        return curBrkpointCodeMap.find(addr) != curBrkpointCodeMap.end();
    }

    void ExtFuncCallHookPtrace::recordOriCode(const size_t &funcID, void *addr, bool isPLT) {
        //Get the plt data of curSymbol
        //todo: .plt size is hard coded
        //Warning: this memory should be freed in ~PltCodeInfo

        brkPointInfo.brkpointCodeMap[addr] = static_cast<unsigned long *>(pmParser.readProcMem(addr,
                                                                                               sizeof(unsigned long)));
        brkPointInfo.brkpointFuncMap[addr] = funcID;

        if (isPLT)
            brkPointInfo.brkpointPltAddr.insert(addr);
    }

    bool ExtFuncCallHookPtrace::isBrkPointLocPlt(void *brkpointLoc) {
        auto &curBreakpointPLTAddr = brkPointInfo.brkpointPltAddr;
        return curBreakpointPLTAddr.find(brkpointLoc) != curBreakpointPLTAddr.end();
    }

    void ExtFuncCallHookPtrace::skipBrkPoint(void *brkpointLoc, user_regs_struct &regs, int childTid,
                                             bool continueAfterSkip) {
        /**
         * Resume execution
         */
        if (brkpointLoc != nullptr) {
            auto &curBrkPointCodeMap = brkPointInfo.brkpointCodeMap;
            // Remove the breakpoint by restoring the previous data at the target address, and unwind the EIP back by 1 to

            unsigned long oriCode = *curBrkPointCodeMap.at(brkpointLoc);
            if (ptrace(PTRACE_POKETEXT, childTid, (void *) brkpointLoc, oriCode) < 0) {
                throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_POKETEXT failed because: %s", strerror(errno));
            }
            // Restore rip to orignal location rip-=1
            regs.rip = reinterpret_cast<unsigned long long int>(brkpointLoc);
            if (ptrace(PTRACE_SETREGS, childTid, 0, &regs) < 0) {
                throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_SETREGS failed because: %s", strerror(errno));
            }
            //Execute one more instruction
            if (ptrace(PTRACE_SINGLESTEP, childTid, 0, 0) < 0) {
                throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_SINGLESTEP failed because: %s", strerror(errno));
            }

            int wait_status;
            //Wait for child to stop on its first instruction
            waitpid(childTid, &wait_status, 0);
            if (WIFSTOPPED(wait_status)) {
                //DBG_LOG("Re-insert breakpoint");
                insertBrkpointAt(brkpointLoc, childTid);
                //DBG_LOG("Continue execution");
                if (continueAfterSkip) {
                    if (ptrace(PTRACE_CONT, childTid, 0, 0) < 0) {
                        throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
                    }
                }
            } else if (WIFEXITED(wait_status)) {
                ERR_LOGS("Child thread %d has already exited, cannot restart it", childTid);
                //remove childID from vector list
                threadExited(childTid);
            } else {
                //Should exit and report error

                DBG_LOGS("WIFSIGNALED: %d", WIFSIGNALED(wait_status));
                DBG_LOGS("WTERMSIG: %d", WTERMSIG(wait_status));
                DBG_LOGS("WSTOPSIG: %d", WSTOPSIG(wait_status));
                DBG_LOGS("WIFCONTINUED: %d", WIFCONTINUED(wait_status));
                DBG_LOGS("WSTOPSIG: %s", strsignal(WSTOPSIG(wait_status)));
                DBG_LOGS("WTERMSIG: %s", strsignal(WTERMSIG(wait_status)));

                DBG_LOGS("%d\n", wait_status >> 8);
                DBG_LOGS("%d\n", (SIGTRAP | PTRACE_EVENT_EXEC << 8));

                assert(false);
            }
        }
    }

    void ExtFuncCallHookPtrace::brkpointEmitted(int childTid) {
        size_t curFileID = 0;
        size_t curFuncID = 0;
        void *callerAddr = nullptr;
        void *oriBrkpointLoc = nullptr;
        struct user_regs_struct regs{};
        if (!parseSymbolInfo(curFileID, curFuncID, callerAddr, oriBrkpointLoc, regs, childTid)) {
            //Brakpoint failed, may cause by sigstop sent by other process, continue.
            //todo: Maybe I should pass signal to target instead
            if (ptrace(PTRACE_CONT, childTid, 0, 0) < 0) {
                throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
            }
            return;
        }

        //Check if the breakpoint loc is a plt address
        if (isBrkPointLocPlt(oriBrkpointLoc)) {
            preHookHandler(curFileID, curFuncID, callerAddr, oriBrkpointLoc, regs, childTid);
        } else {
            afterHookHandler(childTid);
        }
        skipBrkPoint(oriBrkpointLoc, regs, childTid);
    }

    void ExtFuncCallHookPtrace::newThreadCreated(int childTid) {
        //Get newly created threadid
        long newThreadTID = -1;
        if (ptrace(PTRACE_GETEVENTMSG, childTid, 0, &newThreadTID) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_GETEVENTMSG failed because: %s", strerror(errno));
        }

        //Add the new thread to
        tracedTID.insert(newThreadTID);

        //Attach to new thread
        ptrace(PTRACE_SEIZE, newThreadTID, nullptr, nullptr);

        DBG_LOGS("thread tid=%d created new thread tid=%d, new thread is attached", childTid, newThreadTID);

        if (ptrace(PTRACE_CONT, childTid, 0, 0) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
        }
    }

    void ExtFuncCallHookPtrace::threadExited(int childTid) {
        tracedTID.erase(childTid);
    }

    int ExtFuncCallHookPtrace::waitForAllChild(int &waitStatus) {
        bool WAIT_SUCCESS = false;
        while (!WAIT_SUCCESS) {
            for (auto iter = tracedTID.begin(); iter != tracedTID.end(); ++iter) {
                //todo:busywaiting
                int waitRlt = waitpid(*iter, &waitStatus, WNOHANG);
                if (waitRlt < 0) {
                    throwScalerExceptionS(ErrCode::WAIT_FAIL, "WAIT_FAIL failed because: %s", strerror(errno));
                } else if (waitRlt != 0) {
                    WAIT_SUCCESS = true;
                    return *iter;
                }
            }
            if (tracedTID.size() == 0) {
                return -1;
            }
        }
    }

    void *ExtFuncCallHookPtrace::findMainAddress() {
        ELFParser_Linux elfParser(pmParser.curExecAbsolutePath);
        auto strTblInfo = elfParser.getSecHdrByName(".strtab");
        char *strTbl = (char *) elfParser.getSecContent(strTblInfo);
        //todo: ELFParser should convert ELF image to its internal datastructure rather than providing raw functions for other classes to invoke
        auto symTblInfo = elfParser.getSecHdrByName(".symtab");
        ElfW(Sym) *symTabContent = (ElfW(Sym) *) elfParser.getSecContent(symTblInfo);
        size_t symTblSize = symTblInfo.secHdr.sh_size / sizeof(Elf64_Sym);
        //Add mian thread tid to the list, mainthread tid=pid
        tracedTID.insert(childMainThreadTID);
        uint8_t *mainAddr = nullptr;
        for (int i = 0; i < symTblSize; ++i) {
            if (strcmp("main", strTbl + symTabContent[i].st_name) == 0) {
                mainAddr = (uint8_t *) symTabContent[i].st_value;
                break;
            }
        }
        mainAddr += (ElfW(Addr)) pmParser.fileBaseAddrMap.at(0).first;
        return mainAddr;
    }

    void ExtFuncCallHookPtrace::waitBeforeMainExecution() {
        //Insert breakpoint at main
        void *mainAddr = findMainAddress();
        auto *mainCode = static_cast<unsigned long *>(pmParser.readProcMem(mainAddr, sizeof(unsigned long)));
        //brkPointInfo will free memory for mainCode
        brkPointInfo.brkpointCodeMap[mainAddr] = mainCode;

        insertBrkpointAt(mainAddr, childMainThreadTID);

        if (ptrace(PTRACE_CONT, childMainThreadTID, 0, 0) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_CONT failed because: %s", strerror(errno));
        }
        int wait_status;
        //Wait for the child to stop before execution so we can insert breakpoints
        //This is where the child is first loaded into memory
        if (waitpid(childMainThreadTID, &wait_status, 0) < 0) {
            throwScalerExceptionS(ErrCode::WAIT_FAIL, "WAIT_FAIL failed because: %s", strerror(errno));
        }

        if (!WIFSTOPPED(wait_status)) {
            throwScalerExceptionS(ErrCode::WAIT_FAIL, "Child emit signal other than SIG_STOP, but is %s",
                                  strsignal(WSTOPSIG(wait_status)));
        }

        user_regs_struct regs{};
        if (ptrace(PTRACE_GETREGS, childMainThreadTID, 0, &regs) < 0) {
            throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_POKETEXT failed because: %s", strerror(errno));
        }


        //Resotre main
        skipBrkPoint((void *) (regs.rip - 1), regs, childMainThreadTID, false);
    }

    void ExtFuncCallHookPtrace::parseFuncInfo(size_t callerFileID, int64_t fileIDInCaller, void *&funcAddr,
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


    ExtFuncCallHookPtrace::BrkPointInfo::~BrkPointInfo() {
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
