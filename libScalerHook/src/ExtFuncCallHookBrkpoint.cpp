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
#include <util/tool/VMEmulator.h>
#include <exceptions/ErrCode.h>
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>

namespace scaler {

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::instance = nullptr;

#define INSTR_EXEC_AREA_LEN 256

    class ContextBrkpoint {
    public:
        //todo: Initialize using maximum stack size
        std::vector<size_t> extSymbolId;
        std::vector<size_t> fileId;
        //Variables used to determine whether it's called by hook handler or not
        std::vector<void *> callerAddr;
        std::vector<int64_t> timestamp;
        std::vector<pthread_t *> pthreadIdPtr;
        bool inHookHandler = false;

        char instrExecArea[INSTR_EXEC_AREA_LEN];

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

    thread_local ContextBrkpoint brkpointCurContext;

    void ExtFuncCallHookBrkpoint::install(Hook::SYMBOL_FILTER filterCallB) {
        brkpointCurContext.inHookHandler = true;
        memTool = MemoryTool_Linux::getInst();
        pmParser.parsePMMap();

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //Step2: Install signal handler
        installSigIntHandler();


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
                    recordBrkpointInfo(curSymbol.extSymbolId, curSymbol.pltEntry, true);

                    assert(pmParser.fileBaseAddrMap.at(curFileId).first <= curSymbol.pltEntry &&
                           curSymbol.pltEntry < pmParser.fileBaseAddrMap.at(curFileId).second);
                    DBG_LOGS("Instrumented plt code for symbol: %s:%s at:%p",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             curSymbol.pltEntry);
                    //todo: confusing API interface, addr or brkpointinfo?
                    insertBrkpointAt(brkPointInfo.at(curSymbol.pltEntry));

                    curELFImgInfo.hookedExtSymbol[curSymbol.extSymbolId] = curSymbol;
                }
            }
        }

        brkpointCurContext.inHookHandler = false;
    }

    thread_local SerilizableInvocationTree invocationTreeBrkpoint;
    thread_local InvocationTreeNode *curNodeBrkpoint = &invocationTreeBrkpoint.treeRoot;

    ExtFuncCallHookBrkpoint::ExtFuncCallHookBrkpoint()
            : pmParser(), ExtFuncCallHook_Linux(pmParser, *MemoryTool_Linux::getInst()) {
        invocationTreeBrkpoint.libPltHook = this;

    }

    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::getInst() {
        if (!instance)
            instance = new ExtFuncCallHookBrkpoint();
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


    pthread_mutex_t lockBrkpointOp = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

    void ExtFuncCallHookBrkpoint::insertBrkpointAt(Breakpoint &bp) {
        pthread_mutex_lock(&lockBrkpointOp);

        memTool->adjustMemPerm(bp.addr,
                               (uint8_t *) bp.addr + bp.instLen,
                               PROT_READ | PROT_WRITE | PROT_EXEC);

        bp.addr[0] = 0xCC; //Insert 0xCC to the first byte

        memTool->adjustMemPerm(bp.addr,
                               (uint8_t *) bp.addr + bp.instLen,
                               PROT_READ | PROT_EXEC);

        pthread_mutex_unlock(&lockBrkpointOp);
    }


    void
    ExtFuncCallHookBrkpoint::preHookHandler(size_t curFileID, size_t extSymbolId, void *callerAddr, void *brkpointLoc,
                                            pthread_t childTid) {
        auto startTimeStamp = getunixtimestampms();
        brkpointCurContext.inHookHandler = true;

        brkpointCurContext.timestamp.push_back(getunixtimestampms());
        brkpointCurContext.callerAddr.push_back(callerAddr);
        brkpointCurContext.fileId.push_back(curFileID);
        brkpointCurContext.extSymbolId.push_back(extSymbolId);


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
//            recordOriCode(extSymbolId, callerAddr);
//            insertBrkpointAt(callerAddr);
        }


    }

    void
    ExtFuncCallHookBrkpoint::afterHookHandler(pthread_t childTid) {



//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

        if (brkpointCurContext.fileId.size() <= 0) {
            return;
        }

        int64_t startTimestamp = brkpointCurContext.timestamp.at(brkpointCurContext.timestamp.size() - 1);
        brkpointCurContext.timestamp.pop_back();

        void *callerAddr = brkpointCurContext.callerAddr.at(brkpointCurContext.callerAddr.size() - 1);
        brkpointCurContext.callerAddr.pop_back();

        size_t fileId = brkpointCurContext.fileId.at(brkpointCurContext.fileId.size() - 1);
        brkpointCurContext.fileId.pop_back();

        ELFImgInfo &curELFImgInfo = elfImgInfoMap.at(fileId);

        size_t funcId = brkpointCurContext.extSymbolId.at(brkpointCurContext.extSymbolId.size() - 1);
        brkpointCurContext.extSymbolId.pop_back();
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
            pthread_t *pthreadIdPtr = brkpointCurContext.pthreadIdPtr.at(brkpointCurContext.pthreadIdPtr.size() - 1);
            pmParser.readProcMem(pthreadIdPtr, (size_t) pthreadIdPtr);

            DBG_LOGS("[After Hook Param Parser]    pthread_create tid=%lu", *pthreadIdPtr);
        }


    }


    bool
    ExtFuncCallHookBrkpoint::parseSymbolInfo(size_t &curFileID, size_t &curFuncID, void *&callerAddr,
                                             void *&brkpointLoc, ucontext_t *context) {

        auto thiz = ExtFuncCallHookBrkpoint::getInst();

        //Parse symbol info based on rip, rsp
        //todo:only need rip
        //todo: only support 64 bits

        greg_t regRip = context->uc_mcontext.gregs[REG_RIP];
        greg_t regRsp = context->uc_mcontext.gregs[REG_RSP];

        brkpointLoc = (void *) (regRip - 1); // address of breakpoint

        curFileID = thiz->pmParser.findExecNameByAddr(brkpointLoc);

        if (thiz->brkPointInfo.find(brkpointLoc) == thiz->brkPointInfo.end()) {
            ERR_LOGS("Unexpected stop because rip=%p doesn't seem to be caused by hook", brkpointLoc);
            return false;
        }
        //DBG_LOGS("RIP=%lld RSP=%lld TID=%lu\n", regRip, regRsp, pthread_self());
        callerAddr = reinterpret_cast<void *>(regRsp);

        auto &brkpointInfo = thiz->brkPointInfo.at(brkpointLoc);
        curFuncID = brkpointInfo.funcID;

        return true;

    }

    bool ExtFuncCallHookBrkpoint::brkPointInstalledAt(void *addr) {
        //If a breakpoint is installed, then it must be in pltCodeInfoMap::addrFuncMap
        return brkPointInfo.find(addr) != brkPointInfo.end();
    }

    void ExtFuncCallHookBrkpoint::recordBrkpointInfo(const size_t &funcID, void *addr, bool isPLT) {
        //Get the plt data of curSymbol
        //todo: .plt size is hard coded

        brkPointInfo[addr].addr = static_cast<uint8_t *>(addr);
        brkPointInfo[addr].funcID = funcID;

        //Parse instruction
        //todo: what if fail
        VMEmulator::getInstance().getInstrInfo(brkPointInfo[addr]);


        if (isPLT)
            brkpointPltAddr.insert(addr);
    }

    bool ExtFuncCallHookBrkpoint::isBrkPointLocPlt(void *brkpointLoc) {
        return brkpointPltAddr.find(brkpointLoc) != brkpointPltAddr.end();
    }

#define GET_REG_VALUE(ctxt, reg) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg])
#define SET_REG_VALUE(ctxt, reg, new_value) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg] = new_value)

    void ExtFuncCallHookBrkpoint::skipBrkPoint(Breakpoint &bp, ucontext_t *context) {
        //Otherwise we should execute
        //todo: Assert instruction is valid as it is guaranteed in brkpointEmitted

        VMEmulator &emulator = VMEmulator::getInstance();

        xed_uint64_t relativeAddr = (xed_uint64_t) bp.addr;
        bool isapp = pmParser.addrInApplication(bp.addr);
        if (!isapp) {
            //todo: Change everything to intptr
            relativeAddr = (xed_uint64_t) ((intptr_t) bp.addr - (intptr_t) pmParser.sortedSegments[0].second.addrStart);
        }
        emulator.parseOp(bp.xedDecodedInst, relativeAddr, bp.operands, OPERAND_NUMBER, context);

        //Check the type of original code. If it is jmp, then we shouldn't use assembly to execute but should modify rip directly
        if (bp.xiclass >= XED_ICLASS_JB && bp.xiclass <= XED_ICLASS_JZ) {
            bool shouldJmp = false;
            long long flags = context->uc_mcontext.gregs[REG_EFL];
            long cf_mask = 0x0001;
            long pf_mask = 0x0004;
            //long af_mask = 0x0010;
            long zf_mask = 0x0040;
            long sf_mask = 0x0080;
            long of_mask = 0x0800;

            switch (bp.xiclass) {
                case XED_ICLASS_JB: {
                    // (CF=1)
                    shouldJmp = cf_mask & flags;
                    break;
                }
                case XED_ICLASS_JBE: {
                    // CF=1 or ZF=1
                    shouldJmp = (cf_mask & flags) || (zf_mask & flags);
                    break;
                }
                case XED_ICLASS_JCXZ: {
                    // CX register is 0
                    size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                    size_t cx = rcx_value & 0xFFFF;
                    shouldJmp = (cx == 0);
                    break;
                }
                case XED_ICLASS_JECXZ: {
                    // ECX register is 0
                    size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                    size_t ecx = rcx_value & 0xFFFFFFFF;
                    shouldJmp = (ecx == 0);
                    break;
                }
                case XED_ICLASS_JL: {
                    // SF≠ OF
                    shouldJmp = (sf_mask & flags) ^ (of_mask & flags);
                    break;
                }
                case XED_ICLASS_JLE: {
                    // (ZF=1 or SF ≠ OF)
                    shouldJmp = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
                    break;
                }
                case XED_ICLASS_JNB: {
                    // (CF=0)
                    shouldJmp = !(cf_mask & flags);
                    break;
                }
                case XED_ICLASS_JNBE: {
                    // (CF=0 and ZF=0)
                    shouldJmp = !(cf_mask & flags) && !(zf_mask & flags);
                    break;
                }
                case XED_ICLASS_JNL: {
                    // (SF=OF)
                    shouldJmp = !((sf_mask & flags) ^ (of_mask & flags));
                    break;
                }
                case XED_ICLASS_JNLE: {
                    // (ZF=0 and SF=OF)
                    shouldJmp = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
                    break;
                }
                case XED_ICLASS_JNO: {
                    // (OF=0)
                    shouldJmp = !(of_mask & flags);
                    break;
                }
                case XED_ICLASS_JNP: {
                    // (PF=0)
                    shouldJmp = !(pf_mask & flags);
                    break;
                }
                case XED_ICLASS_JNS: {
                    // (SF=0)
                    shouldJmp = !(sf_mask & flags);
                    break;
                }
                case XED_ICLASS_JNZ: {
                    // (ZF=0)
                    shouldJmp = !(zf_mask & flags);
                    break;
                }
                case XED_ICLASS_JO: {
                    // (OF=1)
                    shouldJmp = of_mask & flags;
                    break;
                }
                case XED_ICLASS_JP: {
                    // (PF=1)
                    shouldJmp = pf_mask & flags;
                    break;
                }
                case XED_ICLASS_JRCXZ: {
                    // RCX register is 0
                    size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                    shouldJmp = (rcx_value == 0);
                    break;
                }
                case XED_ICLASS_JS: {
                    // (SF=1)
                    shouldJmp = sf_mask & flags;
                    break;
                }
                case XED_ICLASS_JZ: {
                    // (ZF=1)
                    shouldJmp = zf_mask & flags;
                    break;
                }
                case XED_ICLASS_JMP:
                case XED_ICLASS_JMP_FAR: {
                    shouldJmp = true;
                    break;
                }
                default: {
                    throwScalerException(ErrCode::UNRECOGNIZSBLE_INSTR,
                                         "Instruction is jmp but scaler doesn't recognize it");
                }
            }

            if (shouldJmp) {
                //near jump
                size_t jmpTarget = VMEmulator::getInstance().getOp(bp.operands[0]);
                //far jump
                void *nextInstrAddr = (void *) jmpTarget;

                bool isapp = pmParser.addrInApplication(bp.addr);
                if (xed3_operand_get_nominal_opcode(&bp.xedDecodedInst) != 0xFF && !isapp) {
                    nextInstrAddr = (void *) ((intptr_t) pmParser.sortedSegments[0].second.addrStart + jmpTarget);
                }

                //Change register value and jmp to this instruction directly
                context->uc_mcontext.gregs[REG_RIP] = (intptr_t) nextInstrAddr;
            }
        } else {
            //Normal instruciton. Execute directly using assembly code;
            MemoryTool_Linux::getInst()->adjustMemPerm(brkpointCurContext.instrExecArea,
                                                       (uint8_t *) brkpointCurContext.instrExecArea +
                                                       INSTR_EXEC_AREA_LEN,
                                                       PROT_READ | PROT_WRITE | PROT_EXEC);
            //Copy the first instruction
            memcpy(brkpointCurContext.instrExecArea, bp.oriCode, bp.instLen);
            assert(sizeof(bp.oriCode) < sizeof(brkpointCurContext.instrExecArea) - 1);
            brkpointCurContext.instrExecArea[bp.instLen] = 0xCB;

            //Fill memory address
            asm volatile (
            "movq %[aInstrExecArea],%%r11\n\t"
            "callq *%%r11\n\t"
            :  //output
            : [aInstrExecArea] "rm"(brkpointCurContext.instrExecArea) //input
            : //clobbers
            );
        }


        //DBG_LOG("Return complete");
        return;
    }

    void ExtFuncCallHookBrkpoint::brkpointEmitted(int signum, siginfo_t *siginfo, void *context) {
        DBG_LOG("brkpointEmitted");
        pthread_t tid = pthread_self();
        auto thiz = ExtFuncCallHookBrkpoint::getInst();

        greg_t regRip = ((ucontext_t *) context)->uc_mcontext.gregs[REG_RIP];

        void *pltPtr = (void *) ((uint64_t) regRip - 1); //The plt address where we applied breakpoint to

        if (thiz->brkPointInfo.find(pltPtr) == thiz->brkPointInfo.end()) {
            ERR_LOGS("Cannot find this breakpoint %p in my library. Not set by me?", pltPtr);
        } else if (brkpointCurContext.inHookHandler) {
            DBG_LOG("Function called within libscalerhook. Skip");
            auto &brkpointInfo = thiz->brkPointInfo.at(pltPtr);
            thiz->skipBrkPoint(brkpointInfo, (ucontext_t *) context);
        } else {
            brkpointCurContext.inHookHandler = true;
            //Parse information
            size_t curFileID = 0;
            size_t curFuncID = 0;
            void *callerAddr = nullptr;
            void *oriBrkpointLoc = nullptr;

            if (!parseSymbolInfo(curFileID, curFuncID, callerAddr, oriBrkpointLoc, (ucontext_t *) context)) {
                //Brakpoint failed, may cause by sigstop sent by other process, continue.
                //todo: Maybe I should pass signal to target instead
                ERR_LOG("Cannot parse function info, continue execution");
                exit(-1);
            }

            //Check if the breakpoint loc is a plt address
            if (thiz->isBrkPointLocPlt(oriBrkpointLoc)) {
                thiz->preHookHandler(curFileID, curFuncID, callerAddr, oriBrkpointLoc, tid);
            } else {
                thiz->afterHookHandler(tid);
            }
            DBG_LOG("brkpoint processing finished, skipping breakpoint");

            auto &brkpointInfo = thiz->brkPointInfo.at((void *) (regRip - 1));
            brkpointCurContext.inHookHandler = false;
            thiz->skipBrkPoint(brkpointInfo, static_cast<ucontext_t *>(context));
        }


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

    void ExtFuncCallHookBrkpoint::installSigIntHandler() {

        struct sigaction sig_trap;
        // block the signal, if it is already inside signal handler
        sigemptyset(&sig_trap.sa_mask);
        sigaddset(&sig_trap.sa_mask, SIGTRAP);
        //set sa_sigaction handler
        sig_trap.sa_flags = SA_SIGINFO;
        sig_trap.sa_sigaction = ExtFuncCallHookBrkpoint::brkpointEmitted;
        // register signal handler
        if (sigaction(SIGTRAP, &sig_trap, NULL) == -1) {
            throwScalerException(ErrCode::SIGINT_HANDLER_REG_FAIL, "Cannot succesfully set signal breakpoint");
        }

    }


}


#endif
