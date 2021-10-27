#ifdef __linux

#include <util/hook/ExtFuncCallHookAsm.hh>
#include <util/tool/ProcInfoParser.h>
#include <cstdint>
#include <util/tool/ElfParser.h>
#include <util/tool/FileTool.h>
#include <exceptions/ScalerException.h>
#include <sys/mman.h>
#include <algorithm>
#include <cassert>
#include <elf.h>
#include <util/tool/MemTool.h>
#include <util/tool/Timer.h>
#include <set>
#include <immintrin.h>
#include <thread>
#include <util/tool/Logging.h>
#include <util/tool/Config.h>
#include <util/tool/PthreadParmExtractor.h>
#include <semaphore.h>
#include <util/tool/SemaphoreParmExtractor.h>
#include <exceptions/ErrCode.h>
#include <type/InvocationTree.h>
#include <util/tool/StringTool.h>
#include <nlohmann/json.hpp>
//todo: many functions are too long

extern "C" { ;
struct Context {
    //todo: Initialize using maximum stack size
    scaler::Stack<scaler::SymID> extSymbolId;
    scaler::Stack<scaler::FileID> fileId;
    //Variables used to determine whether it's called by hook handler or not
    scaler::Stack<void *> callerAddr;
    scaler::Stack<int64_t> timestamp;
    scaler::Stack<pthread_t *> pthreadIdPtr;
};

scaler::ExtFuncCallHookAsm *scaler_extFuncCallHookAsm_thiz = nullptr;

thread_local Context curContext;
__thread bool inhookHandler = false;

long long threadLocalOffsetToTCB = 0;

}
namespace scaler {

    //Declare hook handler written in assembly code

    //void __attribute__((naked)) asmHookHandler();

    void asmHookHandlerSec();

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHookAsm *ExtFuncCallHookAsm::instance = nullptr;


    void ExtFuncCallHookAsm::install(Hook::SYMBOL_FILTER filterCallB) {
        //Calcualte the offset between context variable and tid
        inhookHandler = true;
        //todo: Maybe long is enough?
        auto tid = (pthread_t) THREAD_SELF;


        memTool = MemoryTool_Linux::getInst();
        scaler_extFuncCallHookAsm_thiz = this;

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //Step3: Use callback to determine which ID to hook
        std::vector<ExtSymInfo> symbolToHook;
        std::set<FileID> fileToHook;

        for (auto iterFile = elfImgInfoMap.begin(); iterFile != elfImgInfoMap.end(); ++iterFile) {
            auto &curFileId = iterFile.getKey();
            auto &curFileName = pmParser.idFileMap.at(curFileId);
            auto &curElfImgInfo = iterFile.getVal();
            //loop through external symbols, let user decide which symbol to hook through callback function
            for (auto iterSymbol = curElfImgInfo.idFuncMap.begin();
                 iterSymbol != curElfImgInfo.idFuncMap.end(); ++iterSymbol) {
                auto &curSymbolId = iterSymbol->first;
                auto &curSymbolName = iterSymbol->second;

                const auto &curSymbol = curElfImgInfo.allExtSymbol.at(curSymbolId);
                if (curSymbol.type != STT_FUNC || curSymbol.bind != STB_GLOBAL) {
                    continue;
                }

                if (filterCallB(curFileName, curSymbolName)) {
                    //The user wants this symbol
                    symbolToHook.emplace_back(curElfImgInfo.allExtSymbol.at(curSymbolId));
                    fileToHook.emplace(curFileId);

                    //If the function name matches common pthread functions. Store the function id in advance
                    if (curSymbolName == "pthread_create") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_CREATE = curSymbolId;
                    } else if (curSymbolName == "pthread_join") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_JOIN = curSymbolId;
                    } else if (curSymbolName == "pthread_tryjoin_np") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_TRYJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_timedjoin_np") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_TIMEDJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_clockjoin_np") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_CLOCKJOIN_NP = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_lock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_LOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_timedlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_TIMEDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_clocklock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_CLOCKLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_mutex_unlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_rdlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_RDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_tryrdlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_timedrdlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_clockrdlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKRDLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_wrlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_WRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_trywrlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_timedwrlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_clockwrlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKWRLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_rwlock_unlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_signal") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_SIGNAL = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_broadcast") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_BROADCAST = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_wait") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_WAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_timedwait") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_TIMEDWAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_cond_clockwait") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_CLOCKWAIT = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_lock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_LOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_trylock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_TRYLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_spin_unlock") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_UNLOCK = curSymbolId;
                    } else if (curSymbolName == "pthread_barrier_wait") {
                        curElfImgInfo.pthreadExtSymbolId.PTHREAD_BARRIER_WAIT = curSymbolId;
                    }

                    if (curSymbolName == "sem_wait") {
                        curElfImgInfo.semaphoreExtSymbolId.SEM_WAIT = curSymbolId;
                    } else if (curSymbolName == "sem_timedwait") {
                        curElfImgInfo.semaphoreExtSymbolId.SEM_TIMEDWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_clockwait") {
                        curElfImgInfo.semaphoreExtSymbolId.SEM_CLOCKWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_trywait") {
                        curElfImgInfo.semaphoreExtSymbolId.SEM_TRYWAIT = curSymbolId;
                    } else if (curSymbolName == "sem_post") {
                        curElfImgInfo.semaphoreExtSymbolId.SEM_POST = curSymbolId;
                    }
                }
            }
        }

        //Step4: Build pseodo PLT

        //Allocate pseudo address
        /*
        for (auto &curFileID:fileToHook) {
            ELFImgInfo *curELFImgInfo;
            elfImgInfoMap.get(curFileID, curELFImgInfo);

            //Malloc mem area for pseodo plt
            //todo: 16 is related to binary code. I should use a global config file to store it.
            curELFImgInfo->pseudoPlt = (uint8_t *) malloc(curELFImgInfo->allExtSymbol.size() * 16);

            //todo: debug log
            ERR_LOGS("Pseudo PLT addr for %s is %p", curELFImgInfo->filePath.c_str(), curELFImgInfo->pseudoPlt);

            try {
                memTool->adjustMemPerm(curELFImgInfo->pseudoPlt,
                                       curELFImgInfo->pseudoPlt + curELFImgInfo->allExtSymbol.size() * 16,
                                       PROT_READ | PROT_WRITE | PROT_EXEC);
            } catch (const ScalerException &e) {
                ERR_LOGS("Hook Failed, cannot change heap pseudoPlt permission. Exception Info:\"%s\"", e.info.c_str());
                continue;
            }


        }
        */

        //Step5: Write plt handler code to file
        void *redzoneJumperDl = writeAndCompileRedzoneJumper(symbolToHook);


        //todo: for debug only
        void *pseudoPltDl = writeAndCompilePseudoPlt(symbolToHook);

        //Step6: Replace PLT table, jmp to dll function
        for (auto &curSymbol:symbolToHook) {

            try {
                memTool->adjustMemPerm(curSymbol.gotEntry, curSymbol.gotEntry + 1,
                                       PROT_READ | PROT_WRITE | PROT_EXEC);
            } catch (const ScalerException &e) {
                ERR_LOGS("Hook Failed for \"%s\":\"%s\" because %s", pmParser.idFileMap.at(curSymbol.fileId).c_str(),
                         curSymbol.symbolName.c_str(), e.info.c_str());
                continue;
            }

            //Resolve current address
            curSymbol.addr = *curSymbol.gotEntry;

            ELFImgInfo &curELFImgInfo = elfImgInfoMap.get(curSymbol.fileId);

            //Allocate plt table
            //auto binCodeArrPseudoPLT = fillDestAddr2PseudoPltCode(curSymbol.extSymbolId,
            //                                                      curELFImgInfo->pltStartAddr);
            //Copy array todo:16 is machine specific
            //memcpy(curELFImgInfo->pseudoPlt + 16 * curSymbol.extSymbolId, binCodeArrPseudoPLT.data(), 16);

            //Check if address is already resolved
            FileID symbolFileId = pmParser.findExecNameByAddr(curSymbol.addr);
            //Since it's external symbol, it's address must be in another file.
            curELFImgInfo.realAddrResolved[curSymbol.extSymbolId] = (symbolFileId != curSymbol.fileId);

            curELFImgInfo.hookedExtSymbol.put(curSymbol.extSymbolId, curSymbol);
            DBG_LOGS("Added to curELFImgInfo.hookedExtSymbol fileName=%s fileid=%zd symId=%zd",
                     curELFImgInfo.filePath.c_str(), curSymbol.fileId, curSymbol.extSymbolId,
                     curSymbol.extSymbolId);

        }

        /**
         * Replace PLT (.plt, .plt.sec) entries
         */
        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter.getVal();


            for (auto &curSymbol: curELFImgInfo.hookedExtSymbol) {
                //jmp to custom handler function
                char output[256];
                std::string funcName = "__%zu_%zu";
                sprintf(output, funcName.c_str(), curSymbol.fileId, curSymbol.extSymbolId);

                void *redzoneJumperAddr = dlsym(redzoneJumperDl, output);
                curSymbol.pseudoPltEntry = dlsym(pseudoPltDl, output);

                if (redzoneJumperAddr == NULL) {
                    throwScalerException(ErrCode::NO_HANDLER_IN_DLL, "Failed to find redzone jumper address from dll");
                }

                auto pltRedirectorCodeArr = fillDestAddr2PltRedirectorCode(redzoneJumperAddr);

                DBG_LOGS("[%s] %s hooked (ID:%d)", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
                         curSymbol.extSymbolId);

                //Step6: Replace .plt.sec and .plt

                if (curELFImgInfo.pltSecStartAddr != nullptr) {
                    //.plt.sec table exists
                    //todo: adjust the permission back after this
                    try {
                        //Save original .plt.sec code
                        void *oriPlt = malloc(16);
                        memcpy(oriPlt, (uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.extSymbolId, 16);
                        curELFImgInfo.oriPltSecCode.put(curSymbol.extSymbolId, oriPlt);

                        memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.extSymbolId,
                                               (uint8_t *) curELFImgInfo.pltSecStartAddr +
                                               16 * (curSymbol.extSymbolId + 1),
                                               PROT_READ | PROT_WRITE | PROT_EXEC);
                        memcpy((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.extSymbolId,
                               pltRedirectorCodeArr.data(), 16);
                    } catch (const ScalerException &e) {
                        ERR_LOGS(".plt.sec replacement Failed for \"%s\":\"%s\" because %s",
                                 pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                                 e.info.c_str());
                        continue;
                    }
                }


                try {
                    //Save original .plt code
                    void *oriPlt = malloc(16);
                    memcpy(oriPlt,
                           (uint8_t *) (uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.extSymbolId + 1), 16);
                    curELFImgInfo.oriPltCode.put(curSymbol.extSymbolId, oriPlt);

                    memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.extSymbolId + 1),
                                           (uint8_t *) curELFImgInfo.pltStartAddr +
                                           16 * (curSymbol.extSymbolId + 1 + 1),
                                           PROT_READ | PROT_WRITE | PROT_EXEC);
                    memcpy((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.extSymbolId + 1),
                           pltRedirectorCodeArr.data(), 16);
                } catch (const ScalerException &e) {
                    ERR_LOGS(".plt replacement Failed for \"%s\":\"%s\" because %s",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             e.info.c_str());
                    continue;
                }

                pltRedirectorCodeArr.clear();

            }

        }
        inhookHandler = false;
    }

    thread_local SerilizableInvocationTree invocationTree;
    thread_local InvocationTreeNode *curNode = &invocationTree.treeRoot;

    ExtFuncCallHookAsm::ExtFuncCallHookAsm() : ExtFuncCallHook_Linux(pmParser,
                                                                     *MemoryTool_Linux::getInst()) {
        invocationTree.libPltHook = this;
    }

    ExtFuncCallHookAsm *ExtFuncCallHookAsm::getInst() {
        if (!instance)
            instance = new ExtFuncCallHookAsm();
        return instance;
    }


    void ExtFuncCallHookAsm::uninstall() {
        inhookHandler = true;

        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter.getVal();

            for (auto &curSymbol: curELFImgInfo.hookedExtSymbol) {

//                DBG_LOGS("[%s] %s hooked (ID:%d)\n", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
//                         curSymbol.extSymbolId);
                void *oriCodePtr = nullptr;

                if (curELFImgInfo.pltSecStartAddr != nullptr) {
                    //.plt.sec table exists
                    try {
                        //todo: what is this doesn't exist (for example, installer failed at this symbol)
                        void *dataPtr = curELFImgInfo.oriPltSecCode.get(curSymbol.extSymbolId);

                        //todo: adjust the permission back after this
                        memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.extSymbolId,
                                               (uint8_t *) curELFImgInfo.pltSecStartAddr +
                                               16 * (curSymbol.extSymbolId + 1),
                                               PROT_READ | PROT_WRITE | PROT_EXEC);
                        memcpy((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.extSymbolId, dataPtr, 16);
                    } catch (const ScalerException &e) {
                        ERR_LOGS(".plt.sec replacement Failed for \"%s\":\"%s\" because %s",
                                 pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                                 e.info.c_str());
                        continue;
                    }
                }


                try {
                    //todo: what is this doesn't exist (for example, installer failed at this symbol)
                    void *dataPtr = curELFImgInfo.oriPltCode.get(curSymbol.extSymbolId);

                    memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.extSymbolId + 1),
                                           (uint8_t *) curELFImgInfo.pltStartAddr +
                                           16 * (curSymbol.extSymbolId + 1 + 1),
                                           PROT_READ | PROT_WRITE | PROT_EXEC);
                    memcpy((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.extSymbolId + 1), dataPtr, 16);
                } catch (const ScalerException &e) {
                    ERR_LOGS(".plt replacement Failed for \"%s\":\"%s\" because %s",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             e.info.c_str());
                    continue;
                }
                //todo: release memory stored in oroginal plt code
            }

        }

        inhookHandler = false;
    }

    std::vector<uint8_t> ExtFuncCallHookAsm::fillDestAddr2PltRedirectorCode(void *funcAddr) {
        //The following "magical" numbers are actually two instructions
        //"movq  $0xFFFFFFFF,%r11\n\t"   73, 187, 0, 0, 0, 0, 0, 0, 0, 0,
        //"callq *%r11\n\t" 65, 255, 211
        //"jmpq *%r11\n\t" 65, 255, 227 //Jump to dynamic symbol

        //After calling r15. The address of next instruction will be on stack. The hander will pop this address and
        //Calculating function ID by comparing it with the starting address of .plt/.plt.sec address

        std::vector<uint8_t> binCodeArr = {73, 187, 0, 0, 0, 0, 0, 0, 0, 0, 65, 255, 227, 144, 144, 144};

        //funcAddr cannot be placed into one byte. We need to convert funcAddr to binary code and put corresponding bytes
        //to the correct position (It's machine specific.)

        //Build several bytes
        const uint64_t h1 = 0b00000000000000000000000011111111;
        const uint64_t h2 = h1 << 8;
        const uint64_t h3 = h1 << 16;
        const uint64_t h4 = h1 << 24;
        const uint64_t h5 = h1 << 32;
        const uint64_t h6 = h1 << 40;
        const uint64_t h7 = h1 << 48;
        const uint64_t h8 = h1 << 56;

        auto _funcAddr = (ElfW(Addr)) funcAddr;

        //Put funcAddr to corresponding position
        binCodeArr[2] = _funcAddr & h1;
        binCodeArr[3] = (_funcAddr & h2) >> 8;
        binCodeArr[4] = (_funcAddr & h3) >> 16;
        binCodeArr[5] = (_funcAddr & h4) >> 24;
        binCodeArr[6] = (_funcAddr & h5) >> 32;
        binCodeArr[7] = (_funcAddr & h6) >> 40;
        binCodeArr[8] = (_funcAddr & h7) >> 48;
        binCodeArr[9] = (_funcAddr & h8) >> 56;

        return binCodeArr;
    }

    //todo: add a check here to compare with compiled result. This is to make sure the generated code is correct.
//    std::vector<uint8_t> ExtFuncCallHookAsm::fillDestAddr2PseudoPltCode(SymID extSymbolId, void *funcAddr) {
    //The following "magical" numbers are actually two instructions
    //pushq    0FFFFh   104, 00, 00, 00, 00,
    //movq     r11, 0FFFFFFFFh 73, 187, 00, 00, 00, 00, 00, 00, 00, 00,
    //jmpq     r11 65, 255, 227

//        char binCodeArr[] = {255, 52, 37, 00, 00, 00, 00, 73, 199, 195, 00, 00, 00, 00, 65, 255, 227, 00};
//
//        const uint64_t h1 = 0b00000000000000000000000011111111;
//        const uint64_t h2 = h1 << 8;
//        const uint64_t h3 = h1 << 16;
//        const uint64_t h4 = h1 << 24;
//        const uint64_t h5 = h1 << 32;
//        const uint64_t h6 = h1 << 40;
//        const uint64_t h7 = h1 << 48;
//        const uint64_t h8 = h1 << 56;
//
//
//        binCodeArr[1] = extSymbolId & h1;
//        binCodeArr[2] = (extSymbolId & h2) >> 8;
//        binCodeArr[3] = (extSymbolId & h3) >> 16;
//        binCodeArr[4] = (extSymbolId & h4) >> 24;
//
//        auto _funcAddr = (ElfW(Addr)) funcAddr;
//
//        binCodeArr[7] = _funcAddr & h1;
//        binCodeArr[8] = (_funcAddr & h2) >> 8;
//        binCodeArr[9] = (_funcAddr & h3) >> 16;
//        binCodeArr[10] = (_funcAddr & h4) >> 24;
//        binCodeArr[11] = (_funcAddr & h5) >> 32;
//        binCodeArr[12] = (_funcAddr & h6) >> 40;
//        binCodeArr[13] = (_funcAddr & h7) >> 48;
//        binCodeArr[14] = (_funcAddr & h8) >> 56;

//        return binCodeArr;
//    }

    /**
     * todo: This function can be replaced by binary code. But writing this is easier for debugging.
     * Since it's easier to modify.
     *
     * The mission of this function is to skip redzone, save fileID and funcID and jump to the assembly hook handler
     *
     *
     *
     * When this function finishes (The control flow won't return here)
     *
     * todo: Currently, memory alignment is not considered.
     *
     * Input stack
     *
     * oldrsp-152  newrsp-23
     *                      8 bytes caller address
     * oldrsp-145  newrsp-16
     *
     *
     * oldrsp-144  newrsp-15
     *                      8 bytes funcID
     * oldrsp-137  newrsp-8
     *
     *
     * oldrsp-136  newrsp-7
     *                      8 bytes fileID
     * oldrsp-129  newrsp-0
     *
     *
     * oldrsp-128
     *              128 bytes redzone
     * oldrsp-1
     *
     * oldrsp-0        caller(return) address
     */
    void *ExtFuncCallHookAsm::writeAndCompileRedzoneJumper(std::vector<ExtSymInfo> &symbolToHook) {

        FILE *fp = NULL;

        fp = fopen("./redzoneJumper.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");

        for (auto &curSymbol:symbolToHook) {
            ELFImgInfo curElfImgInfo = elfImgInfoMap.get(curSymbol.fileId);


            fprintf(fp, "//%s\n", curElfImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curElfImgInfo.idFuncMap.at(curSymbol.extSymbolId).c_str());

            //todo: In GCC < 8, naked function wasn't supported!!
            fprintf(fp, "void  __attribute__((naked)) __%zu_%zu(){\n", curSymbol.fileId,
                    curSymbol.extSymbolId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            //Store rsp into r11. We'll later use this value to recover rsp to the correct location
            fprintf(fp, "\"movq (%%rsp),%%r11\\n\\t\"\n");
            //Skip 128-bit redzone. //todo: platform specific. Only on System V 64
            fprintf(fp, "\"subq $128,%%rsp\\n\\t\"\n");

            //Now, everything happens after the redzone
            //Store fileID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.fileId);//fileId
            //Store functionID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.extSymbolId);//funcID
            //Store the original stack location into stack
            fprintf(fp, "\"pushq  %%r11\\n\\t\"\n");

            //jmp to assembly hook handler
            fprintf(fp, "\"movq  $%p,%%r11\\n\\t\"\n", asmHookHandlerSec);
            fprintf(fp, "\"jmpq *%%r11\\n\\t\"\n");

            fprintf(fp, ");\n");
            fprintf(fp, "}\n");
        }
        fprintf(fp, "}\n");
        fclose(fp);
        //compile it
        int sysRet = system("gcc-9 -shared -fPIC ./redzoneJumper.cpp -o ./redzoneJumper.so");
        if (sysRet < 0) {
            throwScalerException(ErrCode::COMPILATION_FAILED, "gcc compilation handler failed");
        }


        std::stringstream ss;
        ss << pmParser.curExecPath << "/redzoneJumper.so";
        void *handle = dlopen(ss.str().c_str(),
                              RTLD_NOW);
        if (handle == NULL) {
            throwScalerException(ErrCode::HANDLER_LOAD_FAILED, "dlOpen failed");
        }
        return handle;
    }


    ExtFuncCallHookAsm::~ExtFuncCallHookAsm() {
    }

    void ExtFuncCallHookAsm::parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr,
                                           FileID &libraryFileID) {
        //Find correct symbol
        ELFImgInfo &curELfImgInfo = elfImgInfoMap.get(callerFileID);
        ExtSymInfo &curSymbol = curELfImgInfo.hookedExtSymbol.get(symbolIDInCaller);

        if (curSymbol.symbolName == "exit") {
            int j = 1;
        }
        //Parse address from got table
        curSymbol.addr = *curSymbol.gotEntry;
        funcAddr = curSymbol.addr;
        //Search the fileID
        libraryFileID = pmParser.findExecNameByAddr(curSymbol.addr);
        assert(libraryFileID != -1);
        curSymbol.libraryFileID = libraryFileID;
        DBG_LOGS("curSymbol patched %s lib:%zd", curSymbol.symbolName.c_str(), curSymbol.libraryFileID);
    }

    /**
    * This function dynamically compiles
    *
    * oldrsp-0        caller(return) address
    */
    void *ExtFuncCallHookAsm::writeAndCompilePseudoPlt(std::vector<ExtSymInfo> &symbolToHook) {
        FILE *fp = NULL;

        fp = fopen("./pseudoPlt.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");

        for (auto &curSymbol:symbolToHook) {
            ELFImgInfo &curElfImgInfo = elfImgInfoMap.get(curSymbol.fileId);

            fprintf(fp, "//%s\n", curElfImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curElfImgInfo.idFuncMap.at(curSymbol.extSymbolId).c_str());

            fprintf(fp, "void  __attribute__((naked)) __%zu_%zu(){\n", curSymbol.fileId, curSymbol.extSymbolId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            fprintf(fp, "\"pushq $%zd\\n\\t\"\n", curSymbol.extSymbolId);
            fprintf(fp, "\"movq  $%p,%%r11\\n\\t\"\n", curElfImgInfo.pltStartAddr);
            fprintf(fp, "\"jmpq *%%r11\\n\\t\"\n");

            fprintf(fp, ");\n");
            fprintf(fp, "}\n");
        }
        fprintf(fp, "}\n");
        fclose(fp);
        int sysRet = system("gcc-9 -shared -fPIC ./pseudoPlt.cpp -o ./pseudoPlt.so");
        if (sysRet < 0) {
            throwScalerException(ErrCode::COMPILATION_FAILED, "gcc compilation handler failed");
        }


        std::stringstream ss;
        ss << pmParser.curExecPath << "/pseudoPlt.so";
        void *handle = dlopen(ss.str().c_str(),
                              RTLD_NOW);
        if (handle == NULL) {
            throwScalerException(ErrCode::HANDLER_LOAD_FAILED, "dlOpen failed");
        }
        return handle;
    }

//    void ExtFuncCallHookAsm::saveCommonFuncID() {
//        using Json = nlohmann::json;
//
//        Json outFile;
//        for (auto iterFile = elfImgInfoMap.begin(); iterFile != elfImgInfoMap.end(); ++iterFile) {
//            auto &curFileID = iterFile->first;
//            auto &curELFImgInfo = iterFile->second;
//
//            auto pthreadIds = curELFImgInfo.pthreadFuncId.getAllIds();
//            outFile[std::to_string(curFileID)]["pthread"] = Json();
//            for (int i = 0; i < pthreadIds.size(); ++i) {
//                outFile[std::to_string(curFileID)]["pthread"].push_back(pthreadIds[i]);
//            }
//            auto semaphoreIds = curELFImgInfo.semaphoreFuncId.getAllIds();
//            outFile[std::to_string(curFileID)]["semaphore"] = Json();
//            for (int i = 0; i < semaphoreIds.size(); ++i) {
//                outFile[std::to_string(curFileID)]["semaphore"].push_back(semaphoreIds[i]);
//            }
//        }
//
//        FILE *fp = NULL;
//        fp = fopen("./commonFuncId.json", "w");
//        std::stringstream ss;
//        ss << outFile;
//        fwrite(ss.str().c_str(), 1, ss.str().size(), fp);
//        fclose(fp);
//    }

//I used to put cHookHandler out of scaler namespace
//However, cHookHandler needs to access variables defined in ExtHook_Linux
//If I use friend function, it cannot be called by Assembly code.
//To make code more elegant, I finally put everything in scaler namespace.
//The cost is I have to find compiled function name once
//    IMPL_ASMHANDLER(, _ZN6scaler21ExtFuncCallHookAsm20cPreHookHandlerLinuxEPvS1_)

//    IMPL_ASMHANDLER(Sec, _ZN6scaler21ExtFuncCallHookAsm23cPreHookHandlerLinuxSecEPvS1_)

#define PUSHXMM(ArgumentName) \
"subq $16,%rsp\n\t" \
"vmovdqu  %xmm"#ArgumentName" ,(%rsp)\n\t"

#define POPXMM(ArgumentName) \
"vmovdqu  (%rsp),%xmm"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"


    /**
     * Source code version for #define IMPL_ASMHANDLER
     * We can't add comments to a macro
     *
     * Input stack
     *
     * oldrsp-152  currsp+0
     *                      8 bytes caller address
     * oldrsp-145  currsp+7
     *
     *
     * oldrsp-144  currsp+8
     *                      8 bytes funcID
     * oldrsp-137  currsp+15
     *
     *
     * oldrsp-136  currsp+16
     *                      8 bytes fileID
     * oldrsp-129  currsp+23
     *
     *
     * oldrsp-128
     *              128 bytes redzone
     * oldrsp-1
     *
     * oldrsp-0        caller(return) address
     */
    void __attribute__((naked)) asmHookHandlerSec() {
        //todo: Calculate values based on rsp rathe than saving them to registers
        __asm__ __volatile__ (


        //The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX, R8, R9
        // (R10 is used as a static chain pointer in case of nested functions[25]:21),
        //XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7  are used for the first floating point arguments
        //The wider YMM and ZMM registers

        //If the callee wishes to use registers RBX, RSP, RBP, and R12–R15, it must restore their original

        //Integer return values up to 64 bits in size are stored in RAX up to 128 bit are stored in RAX and RDX

        //For leaf-node functions (functions which do not call any other function(s)), a 128-byte space is stored just
        //beneath the stack pointer of the function. The space is called the red zone. This zone will not be clobbered
        //by any signal or interrupt handlers. Compilers can thus utilize this zone to save local variables.
        //gcc and clang offer the -mno-red-zone flag to disable red-zone optimizations.
        //todo: inefficient code
        //R11 is the only register we can use. Store stack address in it. (%r11)==callerAddr. Later we'll pass this value
        //as a parameter to prehook
        "movq %rsp,%r11\n\t"

        /**
        * Save environment
        */
        //rsp%10h=0
        "pushq %rbx\n\t" //8
        "pushq %rbp\n\t" //8
        "pushq %r12\n\t" //8
        "pushq %r13\n\t" //8
        "pushq %r14\n\t" //8
        "pushq %r15\n\t" //8
        "subq $8,%rsp\n\t" //8
        //        "stmxcsr (%rsp)\n\t" // 4 Bytes(8-4)
        //        "fnstcw 4(%rsp)\n\t" // 2 Bytes(4-2)
        //        "pushf\n\t" //forward flag (Store all)
        "pushq %rax\n\t" //8
        "pushq %rcx\n\t" //8
        "pushq %rdx\n\t" //8
        "pushq %rsi\n\t" //8
        "pushq %rdi\n\t" //8
        "pushq %r8\n\t" //8
        "pushq %r9\n\t" //8
        "pushq %r10\n\t" //8

        "subq $8,%rsp\n\t" //8  (16-byte allignment)

        //rsp%10h=0
        PUSHXMM(0) //16
        PUSHXMM(1) //16
        PUSHXMM(2) //16
        PUSHXMM(3) //16
        PUSHXMM(4) //16
        PUSHXMM(5) //16
        PUSHXMM(6) //16
        PUSHXMM(7) //16

        /**
         * Getting PLT entry address and caller address from stack
         */
        "movq (%r11),%rdx\n\t" //R11 stores callerAddr
        "addq $8,%r11\n\t"
        "movq (%r11),%rsi\n\t" //R11 stores funcID
        "addq $8,%r11\n\t"
        "movq (%r11),%rdi\n\t" //R11 stores fileID

        "movq %rsp,%rcx\n\t"
        "addq $150,%rcx\n\t" //todo: value wrong
        //FileID fileId (rdi), FuncID funcId (rsi), void *callerAddr (rdx), void* oriRspLoc (rcx)

        /**
         * Pre-Hook
         */
        //tips: Use http://www.sunshine2k.de/coding/javascript/onlineelfviewer/onlineelfviewer.html to find the external function name

        //todo: This is error on the server
        "call  cPreHookHandlerLinux\n\t"
        //Save return value to R11. This is the address of real function parsed by handler.
        //The return address is maybe the real function address. Or a pointer to the pseodoPlt table
        "movq %rax,%r11\n\t"


        /**
        * Restore Registers
        */
        POPXMM(7)
        POPXMM(6)
        POPXMM(5)
        POPXMM(4)
        POPXMM(3)
        POPXMM(2)
        POPXMM(1)
        POPXMM(0)

        "addq $8,%rsp\n\t" //8  (16-byte allignment)

        "popq %r10\n\t"
        "popq %r9\n\t"
        "popq %r8\n\t"
        "popq %rdi\n\t"
        "popq %rsi\n\t"
        "popq %rdx\n\t"
        "popq %rcx\n\t"
        "popq %rax\n\t"
        //        "popf\n\t" //forward flag (Store all)

        "addq $8,%rsp\n\t" //8
        //        "ldmxcsr (%rsp)\n\t" // 2 Bytes(8-4)
        //        "fldcw 4(%rsp)\n\t" // 4 Bytes(4-2)

        "popq %r15\n\t"
        "popq %r14\n\t"
        "popq %r13\n\t"
        "popq %r12\n\t"
        "popq %rbp\n\t"
        "popq %rbx\n\t"

        //Restore rsp to original value (Uncomment the following to only test prehook)
        "addq $152,%rsp\n\t"
        "jmpq *%r11\n\t"

        /**
         * Call actual function
         */
        "addq $152,%rsp\n\t"
        "addq $8,%rsp\n\t"
        "callq *%r11\n\t"


        //Save return value to stack
        "pushq %rax\n\t"
        "pushq %rdx\n\t"
        //todo: Handle ZMM

        /**
        * Save Environment
        */
        //        "pushf\n\t"

        "pushq %rdi\n\t"  // currsp=oldrsp-160
        "pushq %rsi\n\t"  // currsp=oldrsp-168
        "pushq %rdx\n\t"  // currsp=oldrsp-176
        "pushq %rcx\n\t"  // currsp=oldrsp-184
        "pushq %r8\n\t"   // currsp=oldrsp-192
        "pushq %r9\n\t"   // currsp=oldrsp-200
        "pushq %r10\n\t"  // currsp=oldrsp-208
        //Save [XYZ]MM[0-7]
        //todo: Also save YMM0-7 and ZMM0-7

        //Save RBX, RSP, RBP, and R12–R15
        "pushq %rbx\n\t" // currsp=oldrsp-600
        "pushq %rsp\n\t" // currsp=oldrsp-608
        "pushq %rbp\n\t" // currsp=oldrsp-616
        "pushq %r12\n\t" // currsp=oldrsp-624
        "pushq %r13\n\t" // currsp=oldrsp-632
        "pushq %r14\n\t" // currsp=oldrsp-640
        "pushq %r15\n\t" // currsp=oldrsp-648
        "pushq %rax\n\t" // currsp=oldrsp-656

        /**
         * Call After Hook
         */
        //todo: This line has compilation error on the server
        "call  cAfterHookHandlerLinux\n\t"
        //Save return value to R11. R11 now has the address of caller.
        "movq %rax,%r11\n\t"

        /**
        * Restore Environment
        */
        "popq %rax\n\t" // currsp=oldrsp-656
        "popq %r15\n\t" // currsp=oldrsp-640
        "popq %r14\n\t" // currsp=oldrsp-632
        "popq %r13\n\t" // currsp=oldrsp-624
        "popq %r12\n\t" // currsp=oldrsp-616
        "popq %rbp\n\t" // currsp=oldrsp-608
        "popq %rsp\n\t" // currsp=oldrsp-600
        "popq %rbx\n\t" // currsp=oldrsp-592


        "popq %r10\n\t" // currsp=oldrsp-200
        "popq %r9\n\t" // currsp=oldrsp-192
        "popq %r8\n\t" // currsp=oldrsp-184
        "popq %rcx\n\t" // currsp=oldrsp-176
        "popq %rdx\n\t" // currsp=oldrsp-168
        "popq %rsi\n\t" // currsp=oldrsp-160
        "popq %rdi\n\t" // currsp=oldrsp-152



        "popq %rdx\n\t"
        "popq %rax\n\t"
        //todo: Handle float return and XMM return


        //Retrun to caller
        "jmpq *%r11\n\t"
        );

    }
}


extern "C" {

/**
 * Calculate the context pointer based on current thread id
 * @return
 */
inline Context *getContext() {
    auto tid = reinterpret_cast<pthread_t>(THREAD_SELF);
    return *(Context **) (threadLocalOffsetToTCB + (long long) tid);
}

//pthread_mutex_t lock0 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static void *cPreHookHandlerLinux(scaler::FileID fileId, scaler::SymID extSymbolId, void *callerAddr, void *rspLoc) {


    //    pthread_mutex_lock(&lock0);
    //todo: The following two values are highly dependent on assembly code
    void *rdiLoc = (uint8_t *) rspLoc - 8;
    void *rsiLoc = (uint8_t *) rspLoc - 16;

    //Calculate fileID
//    assert(curContext->thiz != nullptr);
    auto &_this = scaler_extFuncCallHookAsm_thiz;

    scaler::ExtFuncCallHookAsm::ELFImgInfo &curElfImgInfo = _this->elfImgInfoMap.get(fileId);

    scaler::ExtFuncCallHookAsm::ExtSymInfo &curSymbol = curElfImgInfo.hookedExtSymbol.get(extSymbolId);
    void *retOriFuncAddr = curSymbol.addr;


    if (!curElfImgInfo.realAddrResolved[extSymbolId]) {
        void *curAddr = curSymbol.addr;
        void *newAddr = *curSymbol.gotEntry;
        if (curAddr == newAddr) {
            //todo: 16 depends on opCode array
            retOriFuncAddr = curSymbol.pseudoPltEntry;
        } else {
            curElfImgInfo.realAddrResolved[extSymbolId] = true;
            curSymbol.addr = newAddr;
            retOriFuncAddr = newAddr;
        }
    }

    if (inhookHandler) {
        return retOriFuncAddr;
    }
    //
//    Context *curContext = getContext();
//
//    if (curContext->inHookHandler) {
////        curContext->callerAddr.push(callerAddr);
//        return retOriFuncAddr;
//    }

    //Starting from here, we could call external symbols and it won't cause any problem
    inhookHandler = true;


    auto startTimeStamp = getunixtimestampms();
    //Push callerAddr into stack
//    curContext->timestamp.push(startTimeStamp);
//    curContext->callerAddr.push(callerAddr);
    //Push calling info to afterhook
//    curContext->fileId.push(fileId);
    //todo: rename this to caller function
//    curContext->extSymbolId.push(extSymbolId);


//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

    //std::stringstream ss;
    //ss << 1;
    // uint64_t id = std::stoull(ss.str());

    DBG_LOGS("[Pre Hook] Thread:%lu File(%ld):%s, Func(%ld): %s RetAddr:%p", pthread_self(),
             fileId, _this->pmParser.idFileMap.at(fileId).c_str(),
             extSymbolId, curElfImgInfo.idFuncMap.at(extSymbolId).c_str(), retOriFuncAddr);

    //Parse parameter based on functions
    //todo: for debugging purpose code is not efficient.
    //todo: shorten this
    if (curElfImgInfo.pthreadExtSymbolId.isFuncPthread(extSymbolId)) {
        //Add a tree node
        auto *newNode = new scaler::PthreadInvocationTreeNode();
        newNode->setExtFuncID(extSymbolId);
        newNode->setStartTimestamp(startTimeStamp);
        //Insert at back
        scaler::curNode = scaler::curNode->addChild(newNode);

        if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_CREATE) {
            //todo: A better way is to compare function id rather than name. This is more efficient.
            //todo: A better way is to also compare library id because a custom library will also implement pthread_create.
            pthread_t **newThread;
            scaler::parm_pthread_create(&newThread, rdiLoc);
            curContext.pthreadIdPtr.push(*newThread);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_JOIN) {
            pthread_t *joinThread;
            scaler::parm_pthread_join(&joinThread, rdiLoc);
            newNode->extraField1 = *joinThread;
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_join tid=%lu", pthread_self(),
//                     *joinThread);

        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_TRYJOIN_NP) {
            pthread_t *joinThread;
            scaler::parm_pthread_tryjoin_np(&joinThread, rdiLoc);
            newNode->extraField1 = *joinThread;
//            DBG_LOGS("[Pre Hook Param Parser]   callingthread=%lu pthread_tryjoin_np tid=%lu", pthread_self(),
//                     *joinThread);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_TIMEDJOIN_NP) {
            pthread_t *joinThread;
            scaler::parm_pthread_timedjoin_np(&joinThread, rdiLoc);
            newNode->extraField1 = *joinThread;
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_timedjoin_np tid=%lu", pthread_self(),
//                     *joinThread);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_CLOCKJOIN_NP) {
            pthread_t *joinThread;
            scaler::parm_pthread_clockjoin_np(&joinThread, rdiLoc);
            newNode->extraField1 = *joinThread;
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_clockjoin_np tid=%lu", pthread_self(),
//                     *joinThread);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_LOCK) {
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_mutex_lock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_mutex_lock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_TIMEDLOCK) {
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_mutex_timedlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_mutex_timedlock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_CLOCKLOCK) {
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_mutex_clocklock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_mutex_clocklock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_UNLOCK) {
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_mutex_unlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  parm_pthread_mutex_unlock lID=%p",
//                     pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_RDLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_rdlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_rdlock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYRDLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_tryrdlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_tryrdlock lID=%p",
//                     pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDRDLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_timedrdlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_timedrdlock lID=%p",
//                     pthread_self(), *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKRDLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_clockrdlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_clockrdlock lID=%p",
//                     pthread_self(), *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_WRLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_wrlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_wrlock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYWRLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_trywrlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_trywrlock lID=%p",
//                     pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDWRLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_timedwrlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_timedwrlock lID=%p",
//                     pthread_self(), *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKWRLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_clockwrlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_clockwrlock lID=%p",
//                     pthread_self(), *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_UNLOCK) {
            pthread_rwlock_t **mutex_t;
            scaler::parm_pthread_rwlock_unlock(&mutex_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_rwlock_unlock lID=%p", pthread_self(),
//                     *mutex_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_SIGNAL) {
            pthread_cond_t **cond_t;
            scaler::parm_pthread_cond_signal(&cond_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*cond_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_cond_signal condID=%p", pthread_self(),
//                     *cond_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_BROADCAST) {
            pthread_cond_t **cond_t;
            scaler::parm_pthread_cond_broadcast(&cond_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*cond_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_cond_broadcast condID=%p",
//                     pthread_self(),
//                     *cond_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_WAIT) {
            pthread_cond_t **cond_t;
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_cond_wait(&cond_t, &mutex_t, rdiLoc, rsiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*cond_t);
            newNode->extraField2 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_cond_wait condID=%p", pthread_self(),
//                     *cond_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_TIMEDWAIT) {
            pthread_cond_t **cond_t;
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_cond_timedwait(&cond_t, &mutex_t, rdiLoc, rsiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*cond_t);
            newNode->extraField2 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_cond_timedwait condID=%p",
//                     pthread_self(),
//                     *cond_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_CLOCKWAIT) {
            pthread_cond_t **cond_t;
            pthread_mutex_t **mutex_t;
            scaler::parm_pthread_cond_clockwait(&cond_t, &mutex_t, rdiLoc, rsiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*cond_t);
            newNode->extraField2 = reinterpret_cast<int64_t>(*mutex_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_cond_clockwait condId=%p",
//                     pthread_self(),
//                     *cond_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_LOCK) {
            pthread_spinlock_t **spinlock_t;
            scaler::parm_pthread_spin_lock(&spinlock_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*spinlock_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_spin_lock lID=%p", pthread_self(),
//                     *spinlock_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_TRYLOCK) {
            pthread_spinlock_t **spinlock_t;
            scaler::parm_pthread_spin_trylock(&spinlock_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*spinlock_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_spin_trylock lID=%p", pthread_self(),
//                     *spinlock_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_UNLOCK) {
            pthread_spinlock_t **spinlock_t;
            scaler::parm_pthread_spin_unlock(&spinlock_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*spinlock_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_spin_unlock lID=%p", pthread_self(),
//                     *spinlock_t);
        } else if (extSymbolId == curElfImgInfo.pthreadExtSymbolId.PTHREAD_BARRIER_WAIT) {
            pthread_barrier_t **barrier_t;
            scaler::parm_pthread_barrier_wait(&barrier_t, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*barrier_t);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  pthread_barrier_wait barrierId=%p",
//                     pthread_self(), *barrier_t);
        }
    } else if (curElfImgInfo.semaphoreExtSymbolId.isFuncSemaphore(extSymbolId)) {
        //Add a tree node
        auto *newNode = new scaler::SemaphoreInvocationTreeNode();
        newNode->setExtFuncID(extSymbolId);
        newNode->setStartTimestamp(startTimeStamp);
        //Insert at back
        scaler::curNode = scaler::curNode->addChild(newNode);

        if (extSymbolId == curElfImgInfo.semaphoreExtSymbolId.SEM_WAIT) {
            sem_t **__sem;
            scaler::parm_sem_wait(&__sem, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*__sem);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  sem_wait sID=%p", pthread_self(), *__sem);
        } else if (extSymbolId == curElfImgInfo.semaphoreExtSymbolId.SEM_TIMEDWAIT) {
            sem_t **__sem;
            scaler::parm_sem_timedwait(&__sem, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*__sem);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  sem_timedwait sID=%p", pthread_self(), *__sem);
        } else if (extSymbolId == curElfImgInfo.semaphoreExtSymbolId.SEM_CLOCKWAIT) {
            sem_t **__sem;
            scaler::parm_sem_clockwait(&__sem, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*__sem);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  sem_clockwait sID=%p", pthread_self(), *__sem);
        } else if (extSymbolId == curElfImgInfo.semaphoreExtSymbolId.SEM_TRYWAIT) {
            sem_t **__sem;
            scaler::parm_sem_trywait(&__sem, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*__sem);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  sem_trywait sID=%p", pthread_self(), *__sem);
        } else if (extSymbolId == curElfImgInfo.semaphoreExtSymbolId.SEM_POST) {
            sem_t **__sem;
            scaler::parm_sem_post(&__sem, rdiLoc);
            newNode->extraField1 = reinterpret_cast<int64_t>(*__sem);
//            DBG_LOGS("[Pre Hook Param Parser]    callingthread=%lu  sem_post sID=%p", pthread_self(), *__sem);
        }

    } else {
        //Add a tree node
        auto *newNode = new scaler::InvocationTreeNode();
        newNode->setExtFuncID(extSymbolId);
        newNode->setStartTimestamp(startTimeStamp);
        //Insert at back
        scaler::curNode = scaler::curNode->addChild(newNode);
    }
    //FILE *fp = NULL;
    //fp = fopen("./testHandler.cpp", "w");
    //fclose(fp);

    inhookHandler = false;
//    pthread_mutex_unlock(&lock0);
    return retOriFuncAddr;
}

pthread_mutex_t lock1 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void *cAfterHookHandlerLinux() {
//    Context *curContext = getContext();

//    pthread_mutex_lock(&lock1);
    if (inhookHandler) {
        //todo: Need to bypass this
        void *callerAddr = curContext.callerAddr.peekpop();
//        pthread_mutex_unlock(&lock1);
        return callerAddr;
    }
    inhookHandler = true;

    assert(scaler_extFuncCallHookAsm_thiz != nullptr);
    auto &_this = scaler_extFuncCallHookAsm_thiz;

//        for (int i = 0; i < curContext.fileId.size() * 4; ++i) {
//            printf(" ");
//        }

    scaler::FileID fileId = curContext.fileId.peekpop();

    scaler::ExtFuncCallHookAsm::ELFImgInfo &curELFImgInfo = _this->elfImgInfoMap.get(fileId);

    auto &fileName = curELFImgInfo.filePath;

    scaler::SymID extSymbolID = curContext.extSymbolId.peekpop();

    auto &funcName = curELFImgInfo.idFuncMap.at(extSymbolID);


    int64_t startTimestamp = curContext.timestamp.peekpop();

    int64_t endTimestamp = getunixtimestampms();

    // When after hook is called. Library address is resolved. We use searching mechanism to find the file name.
    // To improve efficiency, we could sotre this value
    void *callerAddr = curContext.callerAddr.peekpop();

    scaler::ExtFuncCallHookAsm::ExtSymInfo &curSymbol = curELFImgInfo.hookedExtSymbol.get(extSymbolID);
    auto libraryFileId = _this->pmParser.findExecNameByAddr(curSymbol.addr);
    auto &libraryFileName = _this->pmParser.idFileMap.at(libraryFileId);
    assert(libraryFileId != -1);
    curSymbol.libraryFileID = libraryFileId;
    scaler::curNode->setRealFileID(libraryFileId);
    scaler::curNode->setFuncAddr(curSymbol.addr);
    scaler::curNode->setEndTimestamp(endTimestamp);
    scaler::curNode = scaler::curNode->getParent();

    DBG_LOGS("[After Hook] Thread ID:%lu Library:%s, Func: %s Start: %ld End: %ld", pthread_self(),
             libraryFileName.c_str(),
             funcName.c_str(), startTimestamp, endTimestamp);

    if (extSymbolID == curELFImgInfo.pthreadExtSymbolId.PTHREAD_CREATE) {
        //todo: A better way is to compare function id rather than name. This is more efficient.
        //todo: A better way is to also compare library id because a custom library will also implement pthread_create.
        pthread_t *pthreadIdPtr = curContext.pthreadIdPtr.peekpop();
        DBG_LOGS("[After Hook Param Parser]    pthread_create tid=%lu", *pthreadIdPtr);
    }


    FILE *fp = NULL;
    fp = fopen("./libScalerhookOutput.csv", "a");
    fprintf(fp, "%lu,%s,%s,%ld,%ld,\n",
            pthread_self(),
            libraryFileName.c_str(),
            funcName.c_str(),
            startTimestamp,
            endTimestamp);
    fclose(fp);


//        if (*curContext.released && curContext.funcId.size() == 0)
//            curContext.realDeconstructor();
//    pthread_mutex_unlock(&lock1);
    inhookHandler = false;
    return callerAddr;
}

}


#endif
