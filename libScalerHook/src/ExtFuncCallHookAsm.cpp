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

//extern "C" {
//#include "xed/xed-interface.h"
//}
//todo: many functions are too long

//#define PREHOOK_ONLY
#define ENABLE_SAVE

struct Context {
    //todo: Initialize using maximum stack size
    scaler::FStack<scaler::SymID, 8192> extSymbolId;
    //Variables used to determine whether it's called by hook handler or not
    scaler::FStack<void *, 8192> callerAddr;
    scaler::FStack<long long, 8192> timeStamp;
    scaler::RawRecordEntry *rawRecord;
    char initializeMe = 0;

};

scaler::ExtFuncCallHookAsm *scaler_extFuncCallHookAsm_thiz = nullptr;

__thread Context *curContext __attribute((tls_model("initial-exec")));

const uint8_t SCALER_TRUE = 145;
const uint8_t SCALER_FALSE = 167;
__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_TRUE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE



long long threadLocalOffsetToTCB = 0;

class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver() {
        bypassCHooks = SCALER_TRUE;
        if (!curContext->rawRecord) {
            fatalError("rawRecord is not initialized");
        }
        char fileName[255];
        DBG_LOGS("Save rawrecord of thread %lu", pthread_self());
        sprintf(fileName, "scaler_time_%lu.bin", pthread_self());
        FILE *fp = NULL;
        //todo: check open success or not
        fp = fopen(fileName, "w");
        if (!fp) {
            fatalErrorS("Fail to create file, reason:%s", strerror(errno));
        }
        const uint64_t &arrSize = scaler_extFuncCallHookAsm_thiz->allExtSymbol.getSize();
        const uint64_t &entrySize = sizeof(scaler::RawRecordEntry);

        if (!fwrite(&arrSize, sizeof(uint64_t), 1, fp)) {
            fatalErrorS("Fail to write data into file, reason:%s", strerror(errno));
        }
        if (!fwrite(&entrySize, sizeof(uint64_t), 1, fp)) {
            fatalErrorS("Fail to write data into file, reason:%s", strerror(errno));
        }
        //Write entire array
        if (!fwrite(curContext->rawRecord, sizeof(scaler::RawRecordEntry), arrSize, fp)) {
            fatalErrorS("Fail to write data into file, reason:%s", strerror(errno));
        }
        //todo: check close success or not
        fclose(fp);
        //Only save once
        delete[] curContext->rawRecord;
        curContext->rawRecord = nullptr;
        free(curContext);
        curContext = nullptr;
    }
};

thread_local DataSaver saverElem;

bool initTLS() {
    //Dont double initialize
    assert(bypassCHooks == SCALER_TRUE);
    //Initialize saving data structure
    //curContext.initializeMe = ~curContext.initializeMe;
    //saverElem.initializeMe = ~saverElem.initializeMe;
    curContext = new Context();
    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }
    curContext->rawRecord = new scaler::RawRecordEntry[scaler_extFuncCallHookAsm_thiz->allExtSymbol.getSize()];
    if (!curContext->rawRecord) {
        fatalError("Failed to allocate memory for RawRecordEntry");
        return false;
    }

    bypassCHooks = SCALER_FALSE;
    return true;
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


    bool ExtFuncCallHookAsm::install(Hook::SYMBOL_FILTER *filterCallB) {
        auto ldPreloadVal = getenv("LD_PRELOAD");
        if (setenv("LD_PRELOAD", "", true) != 0) {
            fatalErrorS("Cannot set environment variable, reason: %s", strerror(errno));
            return false;
        }
        //load plt hook address
        pthread_create_orig = (pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
        if (!pthread_create_orig) {
            fatalError("Cannot find the address of pthread_create");
            return false;
        }

        memTool = MemoryTool_Linux::getInst();
        if (!memTool) {
            return false;
        }
        scaler_extFuncCallHookAsm_thiz = this;

        //Step1: Locating table in memory
        DBG_LOG("Locating required segments from ELF file");
        if (!locateRequiredSecAndSeg()) {
            return false;
        }

        //Step3: Use callback to determine which ID to hook
        for (FileID curFileId = 0; curFileId < elfImgInfoMap.getSize(); ++curFileId) {
            auto &curElfImgInfo = elfImgInfoMap[curFileId];
            DBG_LOGS("PLT start addr for %s is %p", curElfImgInfo.filePath.c_str(), curElfImgInfo.pltStartAddr);

            if (curElfImgInfo.elfImgValid) {
                auto &curFileName = pmParser.idFileMap.at(curFileId);
                //loop through external symbols, let user decide which symbol to hook through callback function
                for (SymID scalerSymbolId:curElfImgInfo.scalerIdMap) {
                    auto &curSymbol = allExtSymbol[scalerSymbolId];

                    if (curSymbol.type != STT_FUNC || curSymbol.bind != STB_GLOBAL) {
                        continue;
                    }

                    if (filterCallB(curFileName, curSymbol.symbolName)) {
                        //The user wants this symbol
                        hookedExtSymbol.pushBack(scalerSymbolId);

                        //ERR_LOGS("Added to curELFImgInfo.hookedExtSymbol fileName=%s fileid=%zd symId=%zd",
                        //         curElfImgInfo.filePath.c_str(), curSymbol.fileId, curSymbol.scalerSymbolId);

                        //todo: adjust for all symbols in advance, rather than do them individually
                        //Adjust permiss for this current got entry
                        if (!memTool->adjustMemPerm(curSymbol.gotEntry, curSymbol.gotEntry + 1,
                                                    PROT_READ | PROT_WRITE | PROT_EXEC)) {

                            ERR_LOGS("Cannot adjust permission for %s's GOT table", curSymbol.symbolName.c_str());
                            continue;
                        }
                    }
                }
            }
        }

        //Step4: Build pseodo PLT
        DBG_LOG("Write and compile pseudo plt");
        void *pseudoPltDl = writeAndCompilePseudoPlt();
        if (!pseudoPltDl) {
            ERR_LOG("Cannot compile pseudo plt table.");
            return false;
        }
        //Step5: Write redzone jumper code to file
        DBG_LOG("Write and compile redzone jumper");
        void *redzoneJumperDl = writeAndCompileRedzoneJumper();
        if (!redzoneJumperDl) {
            ERR_LOG("Cannot compile redzone jumper.");
            return false;
        }
        DBG_LOG("Parse plt entry address");
        if (!parsePltEntryAddress()) {
            ERR_LOG("Cannot parse plt entry address jumper.");
            return false;
        }

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
            if (!redzoneJumperAddr) {
                fatalErrorS("Cannot find redzone jumper address for symbol %s.", funcName.c_str());
                continue;
            }
            curSymbol.pseudoPltEntry = dlsym(pseudoPltDl, output);

            if (!curSymbol.pseudoPltEntry) {
                fatalErrorS("Cannot find pseudo plt address for symbol %s.", funcName.c_str());
                continue;
            }
            std::vector<uint8_t> pltRedirectorCodeArr;
            if (!fillDestAddr2PltRedirectorCode(redzoneJumperAddr, pltRedirectorCodeArr)) {
                fatalErrorS("Cannot find pseudo plt address for symbol %s.", funcName.c_str());
                continue;
            }

            auto &curELFImgInfo = elfImgInfoMap[curSymbol.fileId];
            //DBG_LOGS("[%s] %s hooked (ID:%zd)", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
            //         curSymbol.scalerSymbolId);
            //Step6: Replace .plt.sec or .plt
            if (curELFImgInfo.pltSecStartAddr != nullptr) {
                //.plt.sec table exists
                //todo: adjust the permission back after this
                //Save original .plt.sec code

                curSymbol.oriPltSecCode = malloc(16);
                if (!curSymbol.oriPltSecCode) {
                    fatalErrorS("Failed to allocate memory for %s", curSymbol.symbolName.c_str());
                    return false;
                }
                memcpy(curSymbol.oriPltSecCode, curSymbol.pltSecEntry, 16);

                if (!memTool->adjustMemPerm(
                        curSymbol.pltSecEntry,
                        (uint8_t *) curSymbol.pltSecEntry + 16,
                        PROT_READ | PROT_WRITE | PROT_EXEC)) {
                    ERR_LOGS("Failed to adjust memory permission for %s", curSymbol.symbolName.c_str());
                    continue;
                }

                //Install hook code
                memcpy(curSymbol.pltSecEntry,
                       pltRedirectorCodeArr.data(), 16);
            } else {
                //No .plt.sec, use .plt

                //Save original .plt code

                curSymbol.oriPltCode = malloc(16);
                if (!curSymbol.oriPltCode) {
                    fatalErrorS("Cannot allocate memory for oriPltCode in %s", curSymbol.symbolName.c_str());
                }
                memcpy(curSymbol.oriPltCode, curSymbol.pltEntry, 16);

                if (!memTool->adjustMemPerm(
                        (uint8_t *) curSymbol.pltEntry,
                        (uint8_t *) curSymbol.pltEntry + 16,
                        PROT_READ | PROT_WRITE | PROT_EXEC)) {

                }

                //Install hook code
                memcpy((uint8_t *) curSymbol.pltEntry,
                       pltRedirectorCodeArr.data(), 16);

            }

            pltRedirectorCodeArr.clear();

        }



        //Allocate tls storage, set hook type to FULL
        if (!initTLS()) {
            ERR_LOG("Failed to initialize TLS");
        }
        if (ldPreloadVal) {
            if (!setenv("LD_PRELOAD", ldPreloadVal, true)) {
                fatalErrorS("Cannot set environment variable LD_PRELOAD, reason: %s", strerror(errno));
                return false;
            }
        }
        return true;
    }

    //thread_local SerilizableInvocationTree invocationTree;
    //thread_local InvocationTreeNode *curNode = &invocationTree.treeRoot;

    ExtFuncCallHookAsm::ExtFuncCallHookAsm() : ExtFuncCallHook(pmParser,
                                                               *MemoryTool_Linux::getInst()) {
        //invocationTree.libPltHook = this;
    }

    ExtFuncCallHookAsm *ExtFuncCallHookAsm::getInst() {
        if (!instance) {
            instance = new ExtFuncCallHookAsm();
            if (!instance) {
                fatalError("Cannot allocate memory for ExtFuncCallHookAsm");
                return nullptr;
            }
        }
        return instance;
    }


    bool ExtFuncCallHookAsm::uninstall() {
        //todo: release oriPltCode oriPltSecCode
        //Decallocate recordbuffer for main thread

        for (SymID curSymId: hookedExtSymbol) {
            auto &curSymbol = allExtSymbol[curSymId];

            //DBG_LOGS("[%s] %s hooked (ID:%zd)\n", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
            //curSymbol.extSymbolId);
            void *oriCodePtr = nullptr;

            if (curSymbol.oriPltSecCode != nullptr) {
                //.plt.sec table exists
                //todo: adjust the permission back after this
                if (!memTool->adjustMemPerm(
                        (uint8_t *) curSymbol.pltSecEntry,
                        (uint8_t *) curSymbol.pltSecEntry + 16,
                        PROT_READ | PROT_WRITE | PROT_EXEC)) {
                    ERR_LOG("Cannot adjust memory permission");
                    continue;
                }
                memcpy((uint8_t *) curSymbol.pltSecEntry,
                       curSymbol.oriPltSecCode, 16);
                free(curSymbol.oriPltSecCode);
                curSymbol.oriPltSecCode = nullptr;

            }

            if (curSymbol.oriPltCode != nullptr) {
                //todo: what is this doesn't exist (for example, installer failed at this symbol)
                if (!memTool->adjustMemPerm(
                        (uint8_t *) curSymbol.pltEntry,
                        (uint8_t *) curSymbol.pltEntry + 16,
                        PROT_READ | PROT_WRITE | PROT_EXEC)) {
                    ERR_LOG("Cannot adjust memory permission");
                    continue;
                }
                memcpy((uint8_t *) curSymbol.pltEntry,
                       curSymbol.oriPltCode, 16);
                free(curSymbol.oriPltCode);
                curSymbol.oriPltCode = nullptr;
            }
        }
        return true;
    }


    bool ExtFuncCallHookAsm::fillDestAddr2PltRedirectorCode(void *funcAddr, std::vector<uint8_t> &retPltRedirCode) {
        //The following "magical" numbers are actually two instructions
        //"movq  $0xFFFFFFFF,%r11\n\t"   73, 187, 0, 0, 0, 0, 0, 0, 0, 0,
        //"callq *%r11\n\t" 65, 255, 211
        //"jmpq *%r11\n\t" 65, 255, 227 //Jump to dynamic symbol

        //After calling r15. The address of next instruction will be on stack. The hander will pop this address and
        //Calculating function ID by comparing it with the starting address of .plt/.plt.sec address

        retPltRedirCode = {73, 187, 0, 0, 0, 0, 0, 0, 0, 0, 65, 255, 227, 144, 144, 144};

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
        retPltRedirCode[2] = _funcAddr & h1;
        retPltRedirCode[3] = (_funcAddr & h2) >> 8;
        retPltRedirCode[4] = (_funcAddr & h3) >> 16;
        retPltRedirCode[5] = (_funcAddr & h4) >> 24;
        retPltRedirCode[6] = (_funcAddr & h5) >> 32;
        retPltRedirCode[7] = (_funcAddr & h6) >> 40;
        retPltRedirCode[8] = (_funcAddr & h7) >> 48;
        retPltRedirCode[9] = (_funcAddr & h8) >> 56;

        return true;
    }

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

        std::string execWorkDir;
        if (!getPWD(execWorkDir)) {
            ERR_LOG("Cannot open PWD");
            return nullptr;
        }

        std::stringstream sstream;
        sstream << execWorkDir << "/redzoneJumper-" << getpid() << ".cpp";
        DBG_LOGS("RedzoneJmper location=%s\n", sstream.str().c_str());
        fp = fopen(sstream.str().c_str(), "w");
        if (!fp) {
            fatalError("Cannot open compiled redzone jumper. Check compilation is successful or not.");
            return nullptr;
        }

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
            fprintf(fp,
                    "\"subq $136,%%rsp\\n\\t\"\n"); //128 bits +8 bits=136bits. The extra 8 bits are for alignment

            //Now, everything happens after the redzone
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

        sstream.str("");
        sstream << "gcc -shared -fPIC ";
        sstream << execWorkDir << "/redzoneJumper-" << getpid() << ".cpp ";
        sstream << "-o ";
        sstream << execWorkDir << "/redzoneJumper-" << getpid() << ".so ";

        DBG_LOGS("RedzoneJmper compilation command=%s\n", sstream.str().c_str());

        int sysRet = system(sstream.str().c_str());
        if (sysRet != 0) {
            fatalError("gcc compilation handler failed");
            return nullptr;
        }

        sstream.str("");
        sstream << execWorkDir << "/redzoneJumper-" << getpid() << ".so";
        void *handle = dlopen(sstream.str().c_str(),
                              RTLD_NOW);
        if (handle == NULL) {
            fatalErrorS("Cannot open compiled redzone jumper because: %s", strerror(errno));
            return nullptr;
        }
        return handle;
    }


    ExtFuncCallHookAsm::~ExtFuncCallHookAsm() {
        //todo: release oriPltCode oriPltSecCode
        uninstall();
    }

    bool ExtFuncCallHookAsm::parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr,
                                           FileID &libraryFileID) {
        //Find correct symbol
        ExtSymInfo &curSymbol = allExtSymbol[symbolIDInCaller];

        //Parse address from got table
        funcAddr = curSymbol.addr;
        //Search the fileID
        libraryFileID = pmParser.findExecNameByAddr(curSymbol.addr);

        if (libraryFileID != -1) {
            ERR_LOGS("Cannot find library file id for symbol %s with address %p", curSymbol.symbolName.c_str(),
                     curSymbol.addr);
            return false;
        }
        DBG_LOGS("curSymbol patched %s lib:%zd", curSymbol.symbolName.c_str(), curSymbol.libraryFileID);
        return true;
    }


/**
* This function dynamically compiles
*
* oldrsp-0        caller(return) address
*/
    void *ExtFuncCallHookAsm::writeAndCompilePseudoPlt() {
        FILE *fp = NULL;
        char tmp[PATH_MAX];
        std::string execWorkDir;
        if (!getPWD(execWorkDir)) {
            fatalError("Cannot get present working directory");
            return nullptr;
        }

        std::stringstream sstream;
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".cpp";
        DBG_LOGS("PseudoPlt location=%s\n", sstream.str().c_str());

        fp = fopen(sstream.str().c_str(), "w");
        if (!fp) {
            fatalErrorS("Fail to create file, reason:%s", strerror(errno));
            return nullptr;
        }

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

        sstream.str("");
        sstream << "gcc -shared -fPIC ";
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".cpp ";
        sstream << "-o ";
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".so ";

        int sysRet = system(sstream.str().c_str());
        if (sysRet != 0) {
            fatalError("Pseudo plt compilation failed");
            return nullptr;
        }

        sstream.str("");
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".so";
        void *handle = dlopen(sstream.str().c_str(),
                              RTLD_NOW);
        if (!handle) {
            fatalErrorS("dlOpen failed, reason: %s", strerror(errno));
            return nullptr;
        }
        return handle;
    }

    bool ExtFuncCallHookAsm::parsePltEntryAddress() {
        for (FileID curFileID = 0; curFileID < elfImgInfoMap.getSize(); ++curFileID) {
            auto &curELFImgInfo = elfImgInfoMap[curFileID];
            if (curELFImgInfo.elfImgValid) {
                char *pltAddr = static_cast<char *>(curELFImgInfo.pltStartAddr);
                assert(pltAddr != nullptr);
                char *curAddr = pltAddr;
                int counter = 0;
                for (int _ = 0; _ < curELFImgInfo.scalerIdMap.size(); ++_) {
                    curAddr += 16;
                    ++counter;
                    //todo: use xed to parse operators
                    int *pltStubId = reinterpret_cast<int *>(curAddr + 7);
                    auto &curSymbol = allExtSymbol[curELFImgInfo.scalerIdMap[*pltStubId]];
                    if (curSymbol.symIdInFile != *pltStubId) {
                        fatalError("Plt id parsing logic is flawed.");
                        return false;
                    }
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
        return true;
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

//    POPZMM(7) \
//    POPZMM(6)\
//    POPZMM(5)\
//    POPZMM(4)\
//    POPZMM(3)\
//    POPZMM(2)\
//    POPZMM(1)\
//    POPZMM(0)
#define ASM_RESTORE_ENV_PREHOOK \
    "popq %r10\n\t"\
    "popq %r9\n\t"\
    "popq %r8\n\t"\
    "popq %rdi\n\t"\
    "popq %rsi\n\t"\
    "popq %rdx\n\t"\
    "popq %rcx\n\t"\
    "popq %rax\n\t"


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
        "pushq %rax\n\t" //8
        "pushq %rcx\n\t" //8
        "pushq %rdx\n\t" //8
        "pushq %rsi\n\t" //8
        "pushq %rdi\n\t" //8
        "pushq %r8\n\t" //8
        "pushq %r9\n\t" //8
        "pushq %r10\n\t" //8

        //rsp%10h=0
        //        PUSHZMM(0) //16
        //        PUSHZMM(1) //16
        //        PUSHZMM(2) //16
        //        PUSHZMM(3) //16
        //        PUSHZMM(4) //16
        //        PUSHZMM(5) //16
        //        PUSHZMM(6) //16
        //        PUSHZMM(7) //16

        /**
         * Getting PLT entry address and caller address from stack
         */
        "movq (%r11),%rsi\n\t" //R11 stores callerAddr
        "addq $8,%r11\n\t"
        "movq (%r11),%rdi\n\t" //R11 stores funcID

        //"movq %rsp,%rcx\n\t"
        //"addq $150,%rcx\n\t" //todo: value wrong
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

        "cmpq $1234,%rdi\n\t"
        "jnz  RET_FULL\n\t"

        //=======================================> if rdi==$1234
        "RET_PREHOOK_ONLY:\n\t"
        ASM_RESTORE_ENV_PREHOOK
        //Restore rsp to original value (Uncomment the following to only enable prehook)
        "addq $152,%rsp\n\t"
        "jmpq *%r11\n\t"

        //=======================================> if rdi!=0
        /**
         * Call actual function
         */
        "RET_FULL:\n\t"
        ASM_RESTORE_ENV_PREHOOK
        "addq $152,%rsp\n\t"
        "addq $8,%rsp\n\t" //Override caller address
        "callq *%r11\n\t"

        /**
         * Save return value
         */
        //Save return value to stack
        "pushq %rax\n\t" //8
        "pushq %rdx\n\t" //8
        //                PUSHZMM(0) //16
        //                PUSHZMM(1) //16
        //Save st0
        //                "subq $16,%rsp\n\t" //16
        //                "fstpt (%rsp)\n\t"
        //                "subq $16,%rsp\n\t" //16
        //                "fstpt (%rsp)\n\t"

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
        //                "fldt (%rsp)\n\t"
        //                "addq $16,%rsp\n\t" //16
        //                "fldt (%rsp)\n\t"
        //                "addq $16,%rsp\n\t" //16
        //                POPZMM(1) //16
        //                POPZMM(0) //16
        "popq %rdx\n\t" //8 (Used in afterhook)
        "popq %rax\n\t" //8


        /**
         * Restore callee saved register
         */
        //        "popq %r15\n\t" //8
        //        "popq %r14\n\t" //8
        //        "popq %r13\n\t" //8
        //        "popq %r12\n\t" //8
        //        "popq %rbp\n\t" //8
        //        "popq %rbx\n\t" //8
        //        "ldmxcsr (%rsp)\n\t" // 2 Bytes(8-4)
        //        "fldcw 4(%rsp)\n\t" // 4 Bytes(4-2)
        //        "addq $16,%rsp\n\t" //16


        //"CLD\n\t"
        //Retrun to caller
        "jmpq *%r11\n\t"
        );

    }

}


extern "C" {

static void *cPreHookHandlerLinux(scaler::SymID extSymbolId, void *callerAddr) {

    //todo: The following two values are highly dependent on assembly code
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

    /**
     * No counting, no measuring time
     */
    if (bypassCHooks != SCALER_FALSE) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    }

    //Starting from here, we could call external symbols and it won't cause any problem
    bypassCHooks = SCALER_TRUE;

    DBG_LOGS("[Pre Hook] Thread:%lu File(%ld):%s, Func(%ld): %s RetAddr:%p", pthread_self(),
             curSymbol.fileId, _this->pmParser.idFileMap.at(curSymbol.fileId).c_str(),
             extSymbolId, curSymbol.symbolName.c_str(), retOriFuncAddr);
#ifdef PREHOOK_ONLY
    //skip afterhook
    asm __volatile__ ("movq $1234, %rdi");
    return retOriFuncAddr;
#endif

    /**
    * Counting (Bypass afterhook)
    */
    Context *curContextPtr = curContext;

    assert(curContextPtr->rawRecord != nullptr);

    ++curContextPtr->rawRecord[extSymbolId].counting;

    //Skip afterhook
//    asm volatile ("movq $1234, %%rdi" : /* No outputs. */
//    :/* No inputs. */:"rdi");
//    return retOriFuncAddr;

    /**
    * Record time (Need afterhook)
    */
    curContextPtr->timeStamp.push(getunixtimestampms());
    //Push callerAddr into stack
    curContextPtr->callerAddr.push(callerAddr);
    //Push calling info to afterhook
    //todo: rename this to caller function
    curContextPtr->extSymbolId.push(extSymbolId);

    bypassCHooks = SCALER_FALSE;
    return retOriFuncAddr;
}

pthread_mutex_t lock1 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void *cAfterHookHandlerLinux() {
    bypassCHooks = SCALER_TRUE;
    auto &_this = scaler_extFuncCallHookAsm_thiz;
    Context *curContextPtr = curContext;

    scaler::SymID extSymbolID = curContextPtr->extSymbolId.peekpop();
    //auto &funcName = curELFImgInfo.idFuncMap.at(extSymbolID);
    // When after hook is called. Library address is resolved. We use searching mechanism to find the file name.
    // To improve efficiency, we could sotre this value
    void *callerAddr = curContextPtr->callerAddr.peekpop();
    const long long &preHookTimestamp = curContextPtr->timeStamp.peekpop();
    DBG_LOGS("[After Hook] Thread ID:%lu Func(%ld) End: %llu",
             pthread_self(), extSymbolID, getunixtimestampms() - preHookTimestamp);

    //Save this operation
    assert(curContextPtr->rawRecord != nullptr);
    const long long duration = getunixtimestampms() - preHookTimestamp;
    curContextPtr->rawRecord[extSymbolID].timeStamp = duration;

    if (!curContextPtr->extSymbolId.isEmpty()) {
        curContextPtr->rawRecord[curContextPtr->extSymbolId.peek()].timeStamp -= duration;
    }

    bypassCHooks = SCALER_FALSE;
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
    bypassCHooks = SCALER_TRUE;
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
    bypassCHooks = SCALER_FALSE;
    actualFuncPtr(argData);
    /**
     * Perform required actions after each thread function completes
     */
    return nullptr;
}


// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    bypassCHooks = SCALER_TRUE;
    assert(scaler::pthread_create_orig != nullptr);
    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    // Prepare the inputs for the intermediate (dummy) thread function
    auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
    args->actual_thread_function = start;
    args->data = arg;
    bypassCHooks = SCALER_FALSE;
    // Call the actual pthread_create

    return scaler::pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);
}



//void pthread_exit(void *__retval) {
//    saveRecordBuffer();
//    scaler::pthread_exit_orig(__retval);
//}



}

#endif
