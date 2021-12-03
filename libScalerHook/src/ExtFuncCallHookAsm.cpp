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
#include <util/tool/Logging.h>
#include <util/tool/Config.h>
#include <util/tool/PthreadParmExtractor.h>
#include <semaphore.h>
#include <util/tool/SemaphoreParmExtractor.h>
#include <exceptions/ErrCode.h>
#include <util/tool/StringTool.h>
#include <nlohmann/json.hpp>
#include <util/datastructure/FStack.h>
#include <type/InvocationTree.h>

extern "C" {
#include "xed/xed-interface.h"
}
//todo: many functions are too long

//#define PREHOOK_ONLY
//#define ENABLE_SAVE

struct Context {
    //todo: Initialize using maximum stack size
    scaler::FStack<scaler::SymID, 8192> extSymbolId;
    //Variables used to determine whether it's called by hook handler or not
    scaler::FStack<void *, 8192> callerAddr;
    scaler::Vector<scaler::RawRecordEntry> *recordBuffer;
    char initializeMe = 0;

};

scaler::ExtFuncCallHookAsm *scaler_extFuncCallHookAsm_thiz = nullptr;

thread_local Context curContext;

__thread bool bypassCHooks = true;

long long threadLocalOffsetToTCB = 0;

class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver() {
        bypassCHooks = true;
        assert(curContext.recordBuffer != nullptr);
        char fileName[255];
        DBG_LOGS("Save rawrecord %lu", pthread_self());
        sprintf(fileName, "scaler_rawrecord_%lu.bin", pthread_self());
        FILE *fp = NULL;
        //todo: check open success or not
        fp = fopen(fileName, "w");
        assert(fp != nullptr);
        assert(curContext.recordBuffer != nullptr);
        const uint64_t &arrSize = curContext.recordBuffer->getSize();
        const uint64_t &entrySize = sizeof(scaler::RawRecordEntry);
        fwrite(&arrSize, sizeof(uint64_t), 1, fp);
        fwrite(&entrySize, sizeof(uint64_t), 1, fp);
        //Write entire array
        fwrite(curContext.recordBuffer->data(), sizeof(scaler::RawRecordEntry), curContext.recordBuffer->getSize(), fp);
        //todo: check close success or not
        fclose(fp);
        //Only save once
        delete curContext.recordBuffer;
        curContext.recordBuffer = nullptr;
    }
};

thread_local DataSaver saverElem;

void initTLS() {
    //Dont double initialize
    assert(bypassCHooks == true);
    //Initialize saving data structure
    //curContext.initializeMe = ~curContext.initializeMe;
    //saverElem.initializeMe = ~saverElem.initializeMe;
    curContext.recordBuffer = new scaler::Vector<scaler::RawRecordEntry>(4096);
    bypassCHooks = false;
}

namespace scaler {
    typedef int (*pthread_create_origt)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

    typedef void (*pthread_exit_origt)(void *__retval);

    typedef int (*pthread_cancel_origt)(pthread_t __th);

    pthread_create_origt pthread_create_orig;

    //Declare hook handler written in assembly code

    //void __attribute__((naked)) asmHookHandler();

    void asmHookHandlerSec();

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHookAsm *ExtFuncCallHookAsm::instance = nullptr;


    void ExtFuncCallHookAsm::install(Hook::SYMBOL_FILTER filterCallB) {
        //load plt hook address
        if (!pthread_create_orig) {
            pthread_create_orig = (pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
        }

        memTool = MemoryTool_Linux::getInst();
        scaler_extFuncCallHookAsm_thiz = this;

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //Step3: Use callback to determine which ID to hook
        for (FileID curFileId = 0; curFileId < elfImgInfoMap.getSize(); ++curFileId) {
            auto &curElfImgInfo = elfImgInfoMap[curFileId];
            DBG_LOGS("PLT start addr for %s is %p", curElfImgInfo.filePath.c_str(), curElfImgInfo.pltStartAddr);

            if (curElfImgInfo.elfImgValid) {
                auto &curFileName = pmParser.idFileMap.at(curFileId);
                //loop through external symbols, let user decide which symbol to hook through callback function
                for (SymID scalerSymbolId:curElfImgInfo.scalerIdMap) {
                    auto &curSymbol = allExtSymbol[scalerSymbolId];

                    if (curSymbol.type != STT_FUNC) {
                        continue;
                    }

                    if (filterCallB(curFileName, curSymbol.symbolName)) {
                        //The user wants this symbol
                        hookedExtSymbol.pushBack(scalerSymbolId);

                        //ERR_LOGS("Added to curELFImgInfo.hookedExtSymbol fileName=%s fileid=%zd symId=%zd",
                        //         curElfImgInfo.filePath.c_str(), curSymbol.fileId, curSymbol.scalerSymbolId);

                        //todo: adjust for all symbols in advance, rather than do them individually
                        //Adjust permiss for this current entry
                        try {
                            memTool->adjustMemPerm(curSymbol.gotEntry, curSymbol.gotEntry + 1,
                                                   PROT_READ | PROT_WRITE | PROT_EXEC);
                        } catch (const ScalerException &e) {
                            ERR_LOGS("Hook Failed for \"%s\":\"%s\" because %s",
                                     pmParser.idFileMap.at(curSymbol.fileId).c_str(),
                                     curSymbol.symbolName.c_str(), e.info.c_str());
                            continue;
                        }

                        //Since it's external symbol, it's address must be in another file.


                        //If the function name matches common pthread functions. Store the function id in advance
//                        if (curSymbol.symbolName == "pthread_create") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_CREATE = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_join") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_JOIN = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_tryjoin_np") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_TRYJOIN_NP = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_timedjoin_np") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_TIMEDJOIN_NP = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_clockjoin_np") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_CLOCKJOIN_NP = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_mutex_lock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_LOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_mutex_timedlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_TIMEDLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_mutex_clocklock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_CLOCKLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_mutex_unlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_MUTEX_UNLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_rdlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_RDLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_tryrdlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYRDLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_timedrdlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDRDLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_clockrdlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKRDLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_wrlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_WRLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_trywrlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TRYWRLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_timedwrlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_TIMEDWRLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_clockwrlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_CLOCKWRLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_rwlock_unlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_RWLOCK_UNLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_cond_signal") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_SIGNAL = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_cond_broadcast") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_BROADCAST = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_cond_wait") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_WAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_cond_timedwait") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_TIMEDWAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_cond_clockwait") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_COND_CLOCKWAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_spin_lock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_LOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_spin_trylock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_TRYLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_spin_unlock") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_SPIN_UNLOCK = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "pthread_barrier_wait") {
//                            curElfImgInfo.pthreadExtSymbolId.PTHREAD_BARRIER_WAIT = scalerSymbolId;
//                        }
//
//                        if (curSymbol.symbolName == "sem_wait") {
//                            curElfImgInfo.semaphoreExtSymbolId.SEM_WAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "sem_timedwait") {
//                            curElfImgInfo.semaphoreExtSymbolId.SEM_TIMEDWAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "sem_clockwait") {
//                            curElfImgInfo.semaphoreExtSymbolId.SEM_CLOCKWAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "sem_trywait") {
//                            curElfImgInfo.semaphoreExtSymbolId.SEM_TRYWAIT = scalerSymbolId;
//                        } else if (curSymbol.symbolName == "sem_post") {
//                            curElfImgInfo.semaphoreExtSymbolId.SEM_POST = scalerSymbolId;
//                        }
                    }
                }
            }
        }

        //Step4: Build pseodo PLT
        void *pseudoPltDl = writeAndCompilePseudoPlt();
        //Step5: Write redzone jumper code to file
        void *redzoneJumperDl = writeAndCompileRedzoneJumper();

        parsePltEntryAddress();

        //Step6: Replace PLT table, jmp to dll function
        /**
         * Replace PLT (.plt, .plt.sec) entries
         */

        for (const SymID &hookedSymId:hookedExtSymbol) {
            auto &curSymbol = allExtSymbol[hookedSymId];
            //jmp to custom handler function
            char output[256];
            std::string funcName = "__%zu";
            sprintf(output, funcName.c_str(), curSymbol.scalerSymbolId);

            void *redzoneJumperAddr = dlsym(redzoneJumperDl, output);
            curSymbol.pseudoPltEntry = dlsym(pseudoPltDl, output);

            if (redzoneJumperAddr == NULL) {
                throwScalerException(ErrCode::NO_HANDLER_IN_DLL,
                                     "Failed to find redzone jumper address from dll");
            }

            auto pltRedirectorCodeArr = fillDestAddr2PltRedirectorCode(redzoneJumperAddr);

            auto &curELFImgInfo = elfImgInfoMap[curSymbol.fileId];
            DBG_LOGS("[%s] %s hooked (ID:%zd)", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
                     curSymbol.scalerSymbolId);
            //Step6: Replace .plt.sec and .plt
            if (curELFImgInfo.pltSecStartAddr != nullptr) {
                //.plt.sec table exists
                //todo: adjust the permission back after this
                try {
                    //Save original .plt.sec code

                    curSymbol.oriPltSecCode = malloc(16);
                    memcpy(curSymbol.oriPltSecCode, curSymbol.pltSecEntry, 16);

                    memTool->adjustMemPerm(
                            curSymbol.pltSecEntry,
                            (uint8_t *) curSymbol.pltSecEntry + 16,
                            PROT_READ | PROT_WRITE | PROT_EXEC);

                    //Install hook code
                    memcpy(curSymbol.pltSecEntry,
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

                curSymbol.oriPltCode = malloc(16);
                memcpy(curSymbol.oriPltCode, curSymbol.pltEntry, 16);

                memTool->adjustMemPerm(
                        (uint8_t *) curSymbol.pltEntry,
                        (uint8_t *) curSymbol.pltEntry + 16,
                        PROT_READ | PROT_WRITE | PROT_EXEC);

                //Install hook code
                memcpy((uint8_t *) curSymbol.pltEntry,
                       pltRedirectorCodeArr.data(), 16);
            } catch (const ScalerException &e) {
                ERR_LOGS(".plt replacement Failed for \"%s\":\"%s\" because %s",
                         pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                         e.info.c_str());
                continue;
            }

            pltRedirectorCodeArr.clear();

        }



        //Allocate tls storage, set hook type to FULL
        initTLS();
    }

    //thread_local SerilizableInvocationTree invocationTree;
    //thread_local InvocationTreeNode *curNode = &invocationTree.treeRoot;

    ExtFuncCallHookAsm::ExtFuncCallHookAsm() : ExtFuncCallHook_Linux(pmParser,
                                                                     *MemoryTool_Linux::getInst()) {
        //invocationTree.libPltHook = this;
    }

    ExtFuncCallHookAsm *ExtFuncCallHookAsm::getInst() {
        if (!instance)
            instance = new ExtFuncCallHookAsm();
        return instance;
    }


    void ExtFuncCallHookAsm::uninstall() {
        //todo: release oriPltCode oriPltSecCode
        //Decallocate recordbuffer for main thread

        for (SymID curSymId: hookedExtSymbol) {
            auto &curSymbol = allExtSymbol[curSymId];

            //DBG_LOGS("[%s] %s hooked (ID:%zd)\n", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
            //curSymbol.extSymbolId);
            void *oriCodePtr = nullptr;

            if (curSymbol.oriPltSecCode != nullptr) {
                //.plt.sec table exists
                try {
                    //todo: adjust the permission back after this
                    memTool->adjustMemPerm(
                            (uint8_t *) curSymbol.pltSecEntry,
                            (uint8_t *) curSymbol.pltSecEntry + 16,
                            PROT_READ | PROT_WRITE | PROT_EXEC);
                    memcpy((uint8_t *) curSymbol.pltSecEntry,
                           curSymbol.oriPltSecCode, 16);
                    free(curSymbol.oriPltSecCode);
                    curSymbol.oriPltSecCode = nullptr;
                } catch (const ScalerException &e) {
                    ERR_LOGS(".plt.sec replacement Failed for \"%s\":\"%s\" because %s",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             e.info.c_str());
                    continue;
                }
            }

            if (curSymbol.oriPltCode != nullptr) {
                try {
                    //todo: what is this doesn't exist (for example, installer failed at this symbol)
                    memTool->adjustMemPerm(
                            (uint8_t *) curSymbol.pltEntry,
                            (uint8_t *) curSymbol.pltEntry + 16,
                            PROT_READ | PROT_WRITE | PROT_EXEC);
                    memcpy((uint8_t *) curSymbol.pltEntry,
                           curSymbol.oriPltCode, 16);
                    free(curSymbol.oriPltCode);
                    curSymbol.oriPltCode = nullptr;
                } catch (const ScalerException &e) {
                    ERR_LOGS(".plt replacement Failed for \"%s\":\"%s\" because %s",
                             pmParser.idFileMap.at(curSymbol.fileId).c_str(), curSymbol.symbolName.c_str(),
                             e.info.c_str());
                    continue;
                }
            }
            //todo: release memory stored in oroginal plt code
        }
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
    void *ExtFuncCallHookAsm::writeAndCompileRedzoneJumper() {

        FILE *fp = NULL;

        fp = fopen("./redzoneJumper.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");


        for (auto &curSymbolID:hookedExtSymbol) {
            auto &curSymbol = allExtSymbol[curSymbolID];

#ifdef PRINT_DBG_LOG
            auto &curELFImgInfo = elfImgInfoMap[curSymbol.fileId];
            fprintf(fp, "//%s\n", curELFImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curSymbol.symbolName.c_str());
#endif
            //todo: In GCC < 8, naked function wasn't supported!!
            fprintf(fp, "void  __attribute__((naked)) __%zu(){\n", curSymbol.scalerSymbolId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            //Store rsp into r11. We'll later use this value to recover rsp to the correct location
            fprintf(fp, "\"movq (%%rsp),%%r11\\n\\t\"\n");
            //Skip 128-bit redzone. //todo: platform specific. Only on System V 64
            fprintf(fp, "\"subq $128,%%rsp\\n\\t\"\n");

            //Now, everything happens after the redzone
            //Store fileID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.fileId);//fileId
            //Store functionID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.scalerSymbolId);//funcID
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
        //todo: release oriPltCode oriPltSecCode
        uninstall();
    }

    void ExtFuncCallHookAsm::parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr,
                                           FileID &libraryFileID) {
        //Find correct symbol
        ExtSymInfo &curSymbol = allExtSymbol[symbolIDInCaller];

        if (curSymbol.symbolName == "exit") {
            int j = 1;
        }
        //Parse address from got table
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
    void *ExtFuncCallHookAsm::writeAndCompilePseudoPlt() {
        FILE *fp = NULL;

        fp = fopen("./pseudoPlt.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");

        for (auto &curSymbolID:hookedExtSymbol) {
            auto &curSymbol = allExtSymbol[curSymbolID];
            auto &curELFImgInfo = elfImgInfoMap[curSymbol.fileId];
#ifdef PRINT_DBG_LOG
            fprintf(fp, "//%s\n", curELFImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curSymbol.symbolName.c_str());
#endif

            fprintf(fp, "void  __attribute__((naked)) __%zu(){\n", curSymbol.scalerSymbolId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            fprintf(fp, "\"pushq $%zd\\n\\t\"\n", curSymbol.symIdInFile);
            fprintf(fp, "\"movq  $%p,%%r11\\n\\t\"\n", curELFImgInfo.pltStartAddr);
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

    void ExtFuncCallHookAsm::parsePltEntryAddress() {
        for (FileID curFileID = 0; curFileID < elfImgInfoMap.getSize(); ++curFileID) {
            auto &curELFImgInfo = elfImgInfoMap[curFileID];
            if (curELFImgInfo.elfImgValid) {
                char *pltAddr = static_cast<char *>(curELFImgInfo.pltStartAddr);
                char *curAddr = pltAddr;
                int counter = 0;
                for (int _ = 0; _ < curELFImgInfo.scalerIdMap.size(); ++_) {
                    curAddr += 16;
                    ++counter;
                    //todo: use xed to parse operators
                    int *pltStubId = reinterpret_cast<int *>(curAddr + 7);
                    auto &curSymbol = allExtSymbol[curELFImgInfo.scalerIdMap[*pltStubId]];
                    assert(curSymbol.symIdInFile == *pltStubId);
                    curSymbol.pltEntry = curAddr;
                    curSymbol.pltSecEntry = (char *) curELFImgInfo.pltSecStartAddr;
                    //DBG_LOGS("%s pltStub=%d", curELFImgInfo.filePath.c_str(), *pltStubId);

                    if (isSymbolAddrResolved(curSymbol)) {
//                        DBG_LOGS("%s(%zd):%s(%zd) plt=%p *%p=%p resolved=%s", curELFImgInfo.filePath.c_str(), curFileID,
//                                 curSymbol.symbolName.c_str(), curSymbol.extSymbolId,
//                                 curSymbol.pltEntry,
//                                 curSymbol.gotEntry,
//                                 *curSymbol.gotEntry, "true");
                        curSymbol.addr = *curSymbol.gotEntry;

                    } else {
//                        DBG_LOGS("%s:%s  *%p=%p resolved=%s", curELFImgInfo.filePath.c_str(),
//                                 curSymbol.symbolName.c_str(), curSymbol.gotEntry, *curSymbol.gotEntry, "false");
                        curSymbol.addr = nullptr;
                    }


                }
            }
        }

        for (auto &curSymbol:allExtSymbol) {
            assert(curSymbol.pltEntry != nullptr);
        }
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

#define PUSHZMM(ArgumentName) \
"subq $64,%rsp\n\t" \
"vmovdqu64  %zmm"#ArgumentName" ,(%rsp)\n\t"

#define POPZMM(ArgumentName) \
"vmovdqu64  (%rsp),%zmm"#ArgumentName"\n\t"\
"addq $64,%rsp\n\t"


/**
* Restore Registers
*/
#define ASM_RESTORE_ENV_PREHOOK \
    POPZMM(7) \
    POPZMM(6)\
    POPZMM(5)\
    POPZMM(4)\
    POPZMM(3)\
    POPZMM(2)\
    POPZMM(1)\
    POPZMM(0)\
    "addq $8,%rsp\n\t" \
    \
    "popq %r10\n\t"\
    "popq %r9\n\t"\
    "popq %r8\n\t"\
    "popq %rdi\n\t"\
    "popq %rsi\n\t"\
    "popq %rdx\n\t"\
    "popq %rcx\n\t"\
    "popq %rax\n\t"\
    \
    "ldmxcsr (%rsp)\n\t" \
    "fldcw 4(%rsp)\n\t" \
    "addq $8,%rsp\n\t" \
    \
    "popq %r15\n\t"\
    "popq %r14\n\t"\
    "popq %r13\n\t"\
    "popq %r12\n\t"\
    "popq %rbp\n\t"\
    "popq %rbx\n\t"

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
        //todo: inefficient assembly code
        //R11 is the only register we can use. Store stack address in it. (%r11)==callerAddr. Later we'll pass this value
        //as a parameter to prehook
        "movq %rsp,%r11\n\t"

        /**
        * Save environment
        */
        "pushq %rbx\n\t" //8
        "pushq %rbp\n\t" //8
        "pushq %r12\n\t" //8
        "pushq %r13\n\t" //8
        "pushq %r14\n\t" //8
        "pushq %r15\n\t" //8
        "subq $8,%rsp\n\t" //8
        "stmxcsr (%rsp)\n\t" // 4 Bytes(8-4)
        "fnstcw 4(%rsp)\n\t" // 2 Bytes(4-2)
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
        PUSHZMM(0) //16
        PUSHZMM(1) //16
        PUSHZMM(2) //16
        PUSHZMM(3) //16
        PUSHZMM(4) //16
        PUSHZMM(5) //16
        PUSHZMM(6) //16
        PUSHZMM(7) //16

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

        "cmpq $0,%rdi\n\t"
        "jnz  RET_FULL\n\t"

        //=======================================> if r12==0
        "RET_PREHOOK_ONLY:\n\t"
        ASM_RESTORE_ENV_PREHOOK
        //Restore rsp to original value (Uncomment the following to only enable prehook)
        "addq $152,%rsp\n\t"
        "jmpq *%r11\n\t"

        //=======================================> if r12!=0
        /**
         * Call actual function
         */
        "RET_FULL:\n\t"
        ASM_RESTORE_ENV_PREHOOK
        "addq $152,%rsp\n\t"
        "addq $8,%rsp\n\t" //Override caller address
        "callq *%r11\n\t"

        /**
         * Save callee saved register
         */
        "subq $16,%rsp\n\t" //16
        "stmxcsr (%rsp)\n\t" // 4 Bytes(8-4)
        "fnstcw 4(%rsp)\n\t" // 2 Bytes(4-2)
        "pushq %rbx\n\t" //8
        "pushq %rbp\n\t" //8
        "pushq %r12\n\t" //8
        "pushq %r13\n\t" //8
        "pushq %r14\n\t" //8
        "pushq %r15\n\t" //8

        /**
         * Save return value
         */
        //Save return value to stack
        "pushq %rax\n\t" //8
        "pushq %rdx\n\t" //8
        PUSHZMM(0) //16
        PUSHZMM(1) //16
        //Save st0
        "subq $16,%rsp\n\t" //16
        "fstpt (%rsp)\n\t"
        "subq $16,%rsp\n\t" //16
        "fstpt (%rsp)\n\t"

        /**
         * Call After Hook
         */
        //todo: This line has compilation error on the server
        "call cAfterHookHandlerLinux\n\t"
        //Save return value to R11. R11 now has the address of caller.
        "movq %rax,%r11\n\t"

        /**
        * Restore return value
        */
        "fldt (%rsp)\n\t"
        "addq $16,%rsp\n\t" //16
        "fldt (%rsp)\n\t"
        "addq $16,%rsp\n\t" //16
        POPZMM(1) //16
        POPZMM(0) //16
        "popq %rdx\n\t" //8
        "popq %rax\n\t" //8


        /**
         * Restore callee saved register
         */
        "popq %r15\n\t" //8
        "popq %r14\n\t" //8
        "popq %r13\n\t" //8
        "popq %r12\n\t" //8
        "popq %rbp\n\t" //8
        "popq %rbx\n\t" //8
        "ldmxcsr (%rsp)\n\t" // 2 Bytes(8-4)
        "fldcw 4(%rsp)\n\t" // 4 Bytes(4-2)
        "addq $16,%rsp\n\t" //16


        //"CLD\n\t"
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

inline bool getInHookBoolThreadLocal() {
    auto tid = reinterpret_cast<pthread_t>(THREAD_SELF);
    return *(bool *) (threadLocalOffsetToTCB + (long long) tid);
}


//pthread_mutex_t lock0 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static bool GDB_CTL_LOG = false;

static void *cPreHookHandlerLinux(scaler::FileID fileId, scaler::SymID extSymbolId, void *callerAddr, void *rspLoc) {

    //todo: The following two values are highly dependent on assembly code
    //void *rdiLoc = (uint8_t *) rspLoc - 8;
    //void *rsiLoc = (uint8_t *) rspLoc - 16;

    //Calculate fileID
    auto &_this = scaler_extFuncCallHookAsm_thiz;

    scaler::ExtFuncCallHookAsm::ExtSymInfo &curSymbol = _this->allExtSymbol[extSymbolId];
    void *retOriFuncAddr = curSymbol.addr;

    if (curSymbol.addr == nullptr) {
        //Unresolved
        if (!_this->isSymbolAddrResolved(curSymbol)) {
            //Use ld to resolve
            retOriFuncAddr = curSymbol.pseudoPltEntry;
        } else {
            //Already resolved, but address not updated
            curSymbol.addr = *curSymbol.gotEntry;
            retOriFuncAddr = curSymbol.addr;
        }
    }

    if (bypassCHooks) {
        //Skip afterhook
        asm __volatile__ ("movq $0, %rdi");
        return retOriFuncAddr;
    }

    //Starting from here, we could call external symbols and it won't cause any problem
    bypassCHooks = true;

#ifndef PREHOOK_ONLY
    //Push callerAddr into stack
    curContext.callerAddr.push(callerAddr);
    //Push calling info to afterhook
    //todo: rename this to caller function
    curContext.extSymbolId.push(extSymbolId);

    DBG_LOGS("[Pre Hook] Thread:%lu File(%ld):%s, Func(%ld): %s RetAddr:%p", pthread_self(),
             fileId, _this->pmParser.idFileMap.at(fileId).c_str(),
             extSymbolId, curSymbol.symbolName.c_str(), retOriFuncAddr);

#ifdef ENABLE_SAVE
    //Save this operation
//    assert(curContext.recordBuffer != nullptr);
    if (curContext.recordBuffer != nullptr) {
        curContext.recordBuffer->pushBack(
                scaler::RawRecordEntry(extSymbolId, getunixtimestampms(), scaler::RawRecordEntry::Type::PUSH));
        //DBG_LOG("hit");
    } else {
        DBG_LOG("miss");
    }
#endif

//    if (GDB_CTL_LOG) {
//        printf("%zd:%zd %s\n", curSymbol.fileId, curSymbol.extSymbolId, curSymbol.symbolName.c_str());
//    }

#endif

    bypassCHooks = false;
    return retOriFuncAddr;
}

pthread_mutex_t lock1 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void *cAfterHookHandlerLinux() {
    bypassCHooks = true;
    auto &_this = scaler_extFuncCallHookAsm_thiz;

    scaler::SymID extSymbolID = curContext.extSymbolId.peekpop();
    //auto &funcName = curELFImgInfo.idFuncMap.at(extSymbolID);
    // When after hook is called. Library address is resolved. We use searching mechanism to find the file name.
    // To improve efficiency, we could sotre this value
    void *callerAddr = curContext.callerAddr.peekpop();

    DBG_LOGS("[After Hook] Thread ID:%lu Func(%ld) End: %ld",
             pthread_self(), extSymbolID, getunixtimestampms());
/*
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
*/
#ifdef ENABLE_SAVE
    //Save this operation
    assert(curContext.recordBuffer != nullptr);
    curContext.recordBuffer->pushBack(
            scaler::RawRecordEntry(extSymbolID, getunixtimestampms(), scaler::RawRecordEntry::Type::POP));

#endif
    bypassCHooks = false;
    return callerAddr;

}

}

/**
 * Pthread hook
 */

extern "C" {


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
    args->actual_thread_function(args->data);
    free(args);
    args = nullptr;
    /**
     * Perform required actions after each thread function completes
     */
    return nullptr;
}


// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    bypassCHooks = true;
    assert(scaler::pthread_create_orig != nullptr);
    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    // Prepare the inputs for the intermediate (dummy) thread function
    auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
    args->actual_thread_function = start;
    args->data = arg;
    bypassCHooks = false;
    // Call the actual pthread_create

    return scaler::pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);
}



//void pthread_exit(void *__retval) {
//    saveRecordBuffer();
//    scaler::pthread_exit_orig(__retval);
//}



}

#endif
