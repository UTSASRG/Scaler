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
#include <util/tool/PthreadParmExtractor.h>
#include <semaphore.h>
#include <util/tool/SemaphoreParmExtractor.h>
#include <exceptions/ErrCode.h>
#include <util/tool/StringTool.h>
#include <nlohmann/json.hpp>
#include <util/datastructure/FStack.h>
#include <type/InvocationTree.h>
#include <grpcpp/grpcpp.h>
#include <util/config/Config.h>
#include <grpc/JobServiceGrpc.h>
#include <grpc/ChannelPool.h>
#include <unistd.h>
#include <grpc/ConfigServiceGrpc.h>
#include <fstream>
#include <util/datastructure/Matrix.h>
#include <util/datastructure/MatrixWithLock.h>
//#include <addressbook.pb.h>
//extern "C" {
//#include "xed/xed-interface.h"
//}
//todo: many functions are too long

//#define PREHOOK_ONLY
#define ENABLE_SAVE


class Context {
public:
    //todo: Initialize using maximum stack size
    scaler::FStack<scaler::SymID, 8192> extSymbolId;
    //Variables used to determine whether it's called by hook handler or not
    scaler::FStack<void *, 8192> callerAddr;
    scaler::FStack<long long, 8192> timeStamp;
    char initializeMe = 0;

    //Records which function calls which function for how long, the index is scalerid (Only contains hooked function)
    //todo: Replace timingMatrix to a class
    scaler::Matrix<int64_t> *timingMatrix = nullptr;
    int64_t curThreadNumber = 1; //The default one is main thread

    //Records which symbol is called for how many times, the index is scalerid (Only contains hooked function)
    int64_t *countingVec = nullptr;
    int64_t countingVecRows = -1;
    int64_t layeredRecordNumber[20];

    uint64_t threadCreationTimestamp = 0; //The total execution time of current thread
    bool threadTerminatedPeacefully = false;
    uint64_t threadTerminateTimestamp = 0; //The total execution time of current thread
    short initialized = 0;

    static scaler::MatrixWithLock<int64_t> *sharedTimingMatrix;


    Context(ssize_t libFileSize, ssize_t hookedSymbolSize);

    ~Context();
};

//Be careful, the memory is released in context

const uint8_t SCALER_TRUE = 145;
const uint8_t SCALER_FALSE = 167;

scaler::MatrixWithLock<int64_t> *Context::sharedTimingMatrix = nullptr;

scaler::ExtFuncCallHookAsm *scaler_extFuncCallHookAsm_thiz = nullptr;

Context::Context(ssize_t libFileSize, ssize_t hookedSymbolSize) {
    bool isMainThread = scaler::Config::mainthreadID == pthread_self(); //Dont double initialize

//    for (int i = 0; i < 20; ++i) {
//        layeredRecordNumber[i] = 0;
//    }

    int timingMode = scaler_extFuncCallHookAsm_thiz->getTimingMode();
    if (timingMode == 0) {
        //Execute once a thread is created
        timingMatrix = new scaler::Matrix<int64_t>(hookedSymbolSize, libFileSize);
    } else if (timingMode == 1 || timingMode == 2) {
        if (isMainThread) {
            sharedTimingMatrix = new scaler::MatrixWithLock<int64_t>(hookedSymbolSize, libFileSize);
        }
        //Only execute once
        timingMatrix = sharedTimingMatrix;
    }

    countingVecRows = hookedSymbolSize;
    countingVec = static_cast<int64_t *>(malloc(countingVecRows * sizeof(int64_t)));
    if (!countingVec) {
        fatalError("Failed to allocate memory for countingMatrix");
        exit(-1);
    }

    memset(countingVec, 0, countingVecRows * sizeof(int64_t));
    initialized = SCALER_TRUE;
}

Context::~Context() {

    free(countingVec);
    countingVec = nullptr;

    bool isMainThread = scaler::Config::mainthreadID == pthread_self(); //Dont double initialize

    int timingMode = scaler_extFuncCallHookAsm_thiz->getTimingMode();
    if (timingMode == 0) {
        delete timingMatrix;
        timingMatrix = nullptr;
    } else if (isMainThread && (timingMode == 1 || timingMode == 2)) {
        //Only execute once
        delete timingMatrix;
        timingMatrix = nullptr;
    }
}


__thread Context *curContext __attribute((tls_model("initial-exec")));

__thread bool isThreadCratedByMyself __attribute((tls_model("initial-exec"))) = false;

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_TRUE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE



long long threadLocalOffsetToTCB = 0;
pthread_mutex_t elfInfoUploadLock = PTHREAD_MUTEX_INITIALIZER;
bool elfInfoUploaded = false;

class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver() {
        bypassCHooks = SCALER_TRUE;
        if (!curContext) {
            fatalError("curContext is not initialized, won't save anything");
            return;
        }
        Context *curContexPtr = curContext;
        if (!isThreadCratedByMyself) {
            INFO_LOG("Sending main thread execution time");

            scaler::JobServiceGrpc jobServiceGrpc(scaler::ChannelPool::channel);

            if (!curContexPtr->threadTerminatedPeacefully) {
                INFO_LOGS("Thread %lu termination time is incorrect because it is killed", pthread_self());
                curContexPtr->threadTerminateTimestamp = getunixtimestampms();
            }
            INFO_LOGS("%lu is executed for %lu", pthread_self(),
                      curContexPtr->threadTerminateTimestamp - curContexPtr->threadCreationTimestamp);

            if (!jobServiceGrpc.appendThreadExecTime(getpid(), pthread_self(),
                                                     curContexPtr->threadTerminateTimestamp -
                                                     curContexPtr->threadCreationTimestamp)) {
                ERR_LOG("Cannot send thread total execution time to server");
            }
        }

        pthread_mutex_lock(&elfInfoUploadLock);
        if (!elfInfoUploaded) {
            INFO_LOG("Sending symbol info");
            scaler::JobServiceGrpc jobServiceGrpc(scaler::ChannelPool::channel);
            if (!jobServiceGrpc.appendElfImgInfo(*scaler_extFuncCallHookAsm_thiz)) {
                ERR_LOG("Cannot send elf info to server");
            }
            elfInfoUploaded = true;
        }
        pthread_mutex_unlock(&elfInfoUploadLock);

        if (!isThreadCratedByMyself) {
            std::stringstream ss;
            int64_t curSum = 0;
//            for (int i = 0; i < 20; ++i) {
//                if (curContexPtr->layeredRecordNumber[i] == -1)
//                    break;
//                curSum += curContexPtr->layeredRecordNumber[i];
//                ss << curSum << "\t";
//            }
//            ERR_LOGS("Total thread record: %s",
//                     ss.str().c_str());

            INFO_LOGS("Sending timing info for thread %lu", pthread_self());
            scaler::JobServiceGrpc jobServiceGrpc(scaler::ChannelPool::channel);
            bool isMainThread = scaler::Config::mainthreadID == pthread_self(); //Dont double initialize

            int timingMode = scaler_extFuncCallHookAsm_thiz->getTimingMode();
            if (timingMode == 0) {
                if (!jobServiceGrpc.appendTimingMatrix(getpid(), pthread_self(),
                                                       *curContexPtr->timingMatrix, //Skip index column
                                                       curContexPtr->countingVecRows,
                                                       curContexPtr->countingVec)) {
                    ERR_LOG("Cannot send timing and counting info to server");
                }
            } else if (isMainThread && (timingMode == 1 || timingMode == 2)) {
                //Only execute once
                if (!jobServiceGrpc.appendTimingMatrix(getpid(), pthread_self(),
                                                       *curContexPtr->timingMatrix, //Skip index column
                                                       curContexPtr->countingVecRows,
                                                       curContexPtr->countingVec)) {
                    ERR_LOG("Cannot send timing and counting info to server");
                }
            }


            if (pthread_self() == scaler::Config::mainthreadID) {
                INFO_LOGS("Check report at http://127.0.0.1:8080/analysis/%ld/time", scaler::Config::curJobId);
            }
            delete curContext;
            curContext = nullptr;
        } else {
            curContexPtr->curThreadNumber -= 1;
        }
    }
};

thread_local DataSaver saverElem;

bool initTLS() {
    bool isMainThread = scaler::Config::mainthreadID == pthread_self(); //Dont double initialize

    assert(bypassCHooks == SCALER_TRUE);
    //Initialize saving data structure
    saverElem.initializeMe = 1;
    curContext = new Context(
            scaler_extFuncCallHookAsm_thiz->elfImgInfoMap.getSize(),
            scaler_extFuncCallHookAsm_thiz->hookedExtSymbol.getSize());


    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }

    //RuntimeInfo newInfo;

    bypassCHooks = SCALER_FALSE;
    return true;
}

namespace scaler {
    typedef int (*pthread_create_origt)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

    typedef __pid_t (*fork_origt)(void);

    typedef void (*pthread_exit_origt)(void *__retval);

    typedef int (*pthread_cancel_origt)(pthread_t __th);

    pthread_create_origt pthread_create_orig;

    fork_origt fork_orig;


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


        memTool = MemoryTool_Linux::getInst();
        if (!memTool) {
            return false;
        }
        scaler_extFuncCallHookAsm_thiz = this;

        auto timingModeConfig = Config::globalConf["hook"]["mode"]["timing"].as<std::string>("null");
        INFO_LOGS("The profiling mode is %s", timingModeConfig.c_str());

        if (timingModeConfig == "all") {
            this->timingMode = 0;
        } else if (timingModeConfig == "avg1") {
            this->timingMode = 1;
        } else if (timingModeConfig == "avg2") {
            this->timingMode = 2;
        }

        //Step1: Locating table in memory
        DBG_LOG("Locating required segments from ELF file");
        if (!locateRequiredSecAndSeg()) {
            return false;
        }

        //Record libHookAuto's address range
        bool foundLibHookAuto = false;
        for (int i = 0; i < pmParser.idFileMap.size(); ++i) {
            //INFO_LOGS("%s",pmParser.idFileMap[i].c_str());
            if (strEndsWith(pmParser.idFileMap[i], "libScalerHook-HookAuto.so")) {
                foundLibHookAuto = true;
                Config::libHookStartingAddr = reinterpret_cast<uint64_t>(pmParser.fileBaseAddrMap[i].first);
                Config::libHookEndingAddr = reinterpret_cast<uint64_t>(pmParser.fileBaseAddrMap[i].second);
                break;
            }
        }
        if (!foundLibHookAuto) {
            fatalError("Cannot find the address range of libScalerHook-HookAuto.so is there a name change?")
            exit(-1);
        }

        //Step3: Use callback to determine which ID to hook
        for (FileID curFileId = 0; curFileId < elfImgInfoMap.getSize(); ++curFileId) {
            auto &curElfImgInfo = elfImgInfoMap[curFileId];

            if (curElfImgInfo.elfImgValid) {
                curElfImgInfo.scalerId = numOfHookedELFImg++;
                //loop through external symbols, let user decide which symbol to hook through callback function
                for (SymID scalerSymbolId:curElfImgInfo.scalerIdMap) {
                    auto &curSymbol = allExtSymbol[scalerSymbolId];

                    if (curSymbol.type != STT_FUNC || curSymbol.bind != STB_GLOBAL) {
                        continue;
                    }

                    if (filterCallB(curElfImgInfo.filePath, curSymbol.symbolName)) {
                        //The user wants this symbol
                        curSymbol.hookedId = hookedExtSymbol.getSize();
                        hookedExtSymbol.pushBack(scalerSymbolId);
                        DBG_LOGS("Added to curELFImgInfo.hookedExtSymbol fileName=%s fileid=%zd symId=%zd, %s, %zd",
                                 curElfImgInfo.filePath.c_str(), curSymbol.fileId, curSymbol.scalerSymbolId,
                                 curSymbol.symbolName.c_str(), curSymbol.symIdInFile);

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

//            //Jump endbranch if possible
//            if (*(uint8_t *) curSymbol.pseudoPltEntry == 0xF3) {
//                //Jump endbranch 64
//                curSymbol.pseudoPltEntry = (uint8_t *) curSymbol.pseudoPltEntry + 4;
//            }
//            //Jump endbranch if possible
//            if (*(uint8_t *) redzoneJumperAddr == 0xF3) {
//                //Jump endbranch 64
//                redzoneJumperAddr = (uint8_t *) redzoneJumperAddr + 4;
//            }

//            if( *(uint8_t *) curSymbol.pseudoPltEntry!=0x6A){
//                fatalError("Illegal instruction detected in compiled pseudo PltEntry");
//                return false;
//            }
//            if( *(uint8_t *) redzoneJumperAddr!=0x4c){
//                fatalError("Illegal instruction detected in compiled redzone jumper");
//                return false;
//            }

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
                    //ERR_LOGS("Failed to adjust memory permission for %s", curSymbol.symbolName.c_str());
                    continue;
                }

                //Install hook code
                memcpy(curSymbol.pltSecEntry,
                       pltRedirectorCodeArr.data(), 16);
                DBG_LOGS("File=%s Symbol=%s(ID:%zd) .pltsec=%p hooked", curELFImgInfo.filePath.c_str(),
                         curSymbol.symbolName.c_str(), curSymbol.scalerSymbolId,
                         curSymbol.pltSecEntry);
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
                memcpy(curSymbol.pltEntry,
                       pltRedirectorCodeArr.data(), 16);
                DBG_LOGS("File=%s Symbol=%s(ID:%zd) .plt=%p hooked", curELFImgInfo.filePath.c_str(),
                         curSymbol.symbolName.c_str(), curSymbol.scalerSymbolId,
                         curSymbol.pltEntry);
            }
        }


        bypassCHooks = SCALER_TRUE;
        //Allocate tls storage, set hook type to FULL
        isThreadCratedByMyself = false;

        if (!initTLS()) {
            ERR_LOG("Failed to initialize TLS");
            //This is the main thread
        }
        bypassCHooks = SCALER_FALSE;

        if (ldPreloadVal) {
            setenv("LD_PRELOAD", ldPreloadVal, true);
        }

        installed = true;
        curContext->threadCreationTimestamp = getunixtimestampms();

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
        installed = false;
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

        std::string gccPath = Config::globalConf["hook"]["gccpath"].as<std::string>("null");
        if (gccPath == "null") {
            fatalError("Please make sure hook.gccpath exists in the config file");
            return nullptr;
        }

        sstream.str("");
        sstream << gccPath << " -shared -fPIC ";
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
        uint8_t *handle = static_cast<uint8_t *>(dlopen(sstream.str().c_str(),
                                                        RTLD_NOW));
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
        DBG_LOGS("curSymbol patched %s lib:%zd", curSymbol.symbolName.c_str(), curSymbol.libraryFileScalerID);
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

        std::string gccPath = Config::globalConf["hook"]["gccpath"].as<std::string>("null");
        if (gccPath == "null") {
            fatalError("Please make sure hook.gccpath exists in the config file");
            return nullptr;
        }

        sstream.str("");

        sstream << gccPath << " -shared -fPIC -Werror ";
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".cpp ";
        sstream << "-o ";
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".so ";

        int sysRet = system(sstream.str().c_str());
        if (sysRet != 0) {
            fatalErrorS("Pseudo plt compilation failed with command %s", sstream.str().c_str());
            return nullptr;
        }

        sstream.str("");
        sstream << execWorkDir << "/pseudoPlt-" << getpid() << ".so";
        uint8_t *handle = static_cast<uint8_t *>(dlopen(sstream.str().c_str(),
                                                        RTLD_NOW));
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
                    curAddr += 16; //Skip ld calling plt entry/ move to the next plt entry
                    ++counter;
                    int pushOffset = -1;
                    if (*(uint8_t *) curAddr == 0xFF) {
                        pushOffset = 7;
                    } else if (*(uint8_t *) curAddr == 0xF3) {
                        pushOffset = 5;
                    } else {
                        fatalError("Plt entry format illegal. Cannot find instruction \"push id\"");
                    }

                    //Debug tips: Add breakpoint after this statement, and *pltStubId should be 0 at first, 2 at second .etc
                    int *pltStubId = reinterpret_cast<int *>(curAddr + pushOffset);
                    if (*pltStubId >= (int) curELFImgInfo.scalerIdMap.size()) {
                        fatalErrorS(
                                "Plt entry format illegal. Plt id=%d parsed is larger than the number of symbols=%zu.",
                                *pltStubId, curELFImgInfo.scalerIdMap.size());
                        return false;
                    }
                    auto &curSymbol = allExtSymbol[curELFImgInfo.scalerIdMap[*pltStubId]];
                    if (curSymbol.symIdInFile != *pltStubId) {
                        fatalError("Plt id parsing logic is flawed.");
                        return false;
                    }
                    curSymbol.pltEntry = curAddr;
                    curSymbol.pltSecEntry = (char *) curELFImgInfo.pltSecStartAddr + (*pltStubId) * 16;
                    DBG_LOGS("%s:%s pltStub=%d", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
                             *pltStubId);

                    if (isSymbolAddrResolved(curSymbol)) {
//                        DBG_LOGS("%s(%zd):%s(%zd) plt=%p *%p=%p resolved=%s", curELFImgInfo.filePath.c_str(), curFileID,
//                                 curSymbol.symbolName.c_str(), curSymbol.extSymbolId,
//                                 curSymbol.pltEntry,
//                                 curSymbol.gotEntry,
//                                 *curSymbol.gotEntry, "true");
                        curSymbol.addr = *curSymbol.gotEntry;
                        curSymbol.libraryFileScalerID = pmParser.findExecNameByAddr(curSymbol.addr);
                        assert(curSymbol.libraryFileScalerID != -1);
                    } else {
                        //DBG_LOGS("%s:%s  *%p=%p not resolved=%s", curELFImgInfo.filePath.c_str(),
                        //         curSymbol.symbolName.c_str(), curSymbol.gotEntry, *curSymbol.gotEntry, "false");
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

    bool ExtFuncCallHookAsm::active() {
        return installed;
    }

    int ExtFuncCallHookAsm::getTimingMode() {
        return timingMode;
    }

    void ExtFuncCallHookAsm::updateMainThreadFinishTime(uint64_t timestamp) {
        curContext->threadTerminatedPeacefully = true;
        curContext->threadTerminateTimestamp = timestamp;
        INFO_LOGS("Scaler thinks the app is terminated at %ld", timestamp);
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
    //Starting from here, we could call external symbols and it won't cause any problem

    /**
     * No counting, no measuring time (If scaler is not installed, then tls is not initialized)
     */
    Context *curContextPtr = curContext; //Reduce thread local access
    if (!_this->active() || bypassCHooks != SCALER_FALSE || curContextPtr == nullptr) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    }

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
    assert(curContext != nullptr);
    if (curContextPtr->initialized == SCALER_TRUE) {
        curContextPtr->countingVec[curSymbol.hookedId]++;
    }
    //Skip afterhook
//    asm volatile ("movq $1234, %%rdi" : /* No outputs. */
//    :/* No inputs. */:"rdi");
//    bypassCHooks = SCALER_TRUE;
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
    assert(curContext != nullptr);
//    if (curContextPtr->callerAddr.getSize() <= 20) {
//        curContextPtr->layeredRecordNumber[curContextPtr->callerAddr.getSize() - 1] += 1;
//    } else {
//        curContextPtr->layeredRecordNumber[19] += 1;
//    }

    scaler::SymID extSymbolID = curContextPtr->extSymbolId.peekpop();
    //auto &funcName = curELFImgInfo.idFuncMap.at(extSymbolID)SymInfo.
    void *callerAddr = curContextPtr->callerAddr.peekpop();
    const long long &preHookTimestamp = curContextPtr->timeStamp.peekpop();
    DBG_LOGS("[After Hook] Thread ID:%lu Func(%ld) End: %llu",
             pthread_self(), extSymbolID, getunixtimestampms() - preHookTimestamp);

    //Resolve library id
    scaler::ExtFuncCallHookAsm::ExtSymInfo &curSymbol = _this->allExtSymbol[extSymbolID];
    if (curSymbol.libraryFileScalerID == -1) {
        curSymbol.addr = *curSymbol.gotEntry;
        assert(_this->isSymbolAddrResolved(curSymbol));
        assert(curSymbol.addr != nullptr);
        //Resolve address
        curSymbol.libraryFileScalerID = _this->pmParser.findExecNameByAddr(curSymbol.addr);
        assert(curSymbol.libraryFileScalerID != -1);
    }

    //Save this operation
    const long long duration = getunixtimestampms() - preHookTimestamp;
    if (curContextPtr->initialized == SCALER_TRUE) {

        curContextPtr->timingMatrix->get(curSymbol.hookedId, curSymbol.libraryFileScalerID) += duration;
        if (!curContextPtr->extSymbolId.isEmpty()) {
            scaler::ExtFuncCallHookAsm::ExtSymInfo &parentSym = _this->allExtSymbol[curContextPtr->extSymbolId.peek()];
            if (parentSym.libraryFileScalerID == -1) {
                parentSym.addr = *parentSym.gotEntry;
                assert(_this->isSymbolAddrResolved(parentSym));
                assert(parentSym.addr != nullptr);
                //Resolve address
                parentSym.libraryFileScalerID = _this->pmParser.findExecNameByAddr(
                        parentSym.addr);
                assert(parentSym.libraryFileScalerID != -1);
            }
            curContextPtr->timingMatrix->get(parentSym.hookedId, parentSym.libraryFileScalerID) -= duration;
        }
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

    Context *curContextPtr = curContext;
    if (scaler::Config::libHookStartingAddr <= reinterpret_cast<uint64_t>(actualFuncPtr) &&
        reinterpret_cast<uint64_t>(actualFuncPtr) <= scaler::Config::libHookEndingAddr) {
        //This thread is created by the hook itself, we don't save anything
        DBG_LOGS("thread %lu is not created by myself", pthread_self());
        isThreadCratedByMyself = true;
    } else {
        Context *curContextPtr = curContext;
        isThreadCratedByMyself = false;
        curContextPtr->curThreadNumber += 1;
        DBG_LOGS("thread %lu is created by myself", pthread_self());
    }

    curContextPtr->threadCreationTimestamp = getunixtimestampms();
    actualFuncPtr(argData);
    /**
     * Perform required actions after each thread function completes
     */
    curContextPtr->threadTerminatedPeacefully = true;
    curContextPtr->threadTerminateTimestamp = getunixtimestampms();

    return nullptr;
}


// Main Pthread wrapper functions.
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg) {
    bypassCHooks = SCALER_TRUE;
    //register uint64_t rbp asm ("rbp");
    //register uint64_t rsp asm ("rsp");
    //void **callerAddr1 = reinterpret_cast<void **>(rbp+8);
    //void *callerAddrPtr = *reinterpret_cast<void **>(rsp + 0x8 + 0x40);
    //uint8_t callOpCode = *reinterpret_cast<uint8_t *>((uint64_t) callerAddrPtr - 0x5);
    //if (callOpCode != 0xE8 && callOpCode != 0xFF && callOpCode != 0x9A) {
    //    fatalError("Failed to parse the caller address for pthread_create")
    //    exit(-1);
    //}
    DBG_LOGS("%lu created a thread pthread_create", pthread_self());

    if (scaler::pthread_create_orig == nullptr) {
        //load plt hook address
        scaler::pthread_create_orig = (scaler::pthread_create_origt) dlsym(RTLD_NEXT, "pthread_create");
        if (!scaler::pthread_create_orig) {
            fatalError("Cannot find the address of pthread_create");
            return false;
        }
    }

    auto threadID = pthread_self();
    DBG_LOGS("pthread_create %lu", pthread_self());

    if (scaler::ExtFuncCallHookAsm::getInst()->active()) {
        // Prepare the inputs for the intermediate (dummy) thread function
        auto args = (struct dummy_thread_function_args *) malloc(sizeof(struct dummy_thread_function_args));
        args->actual_thread_function = start;
        args->data = arg;
        bypassCHooks = SCALER_FALSE;
        // Call the actual pthread_create
        return scaler::pthread_create_orig(thread, attr, dummy_thread_function, (void *) args);
    } else {
        //Asm hook not ready
        bypassCHooks = SCALER_FALSE;
        isThreadCratedByMyself = true;
        return scaler::pthread_create_orig(thread, attr, start, (void *) arg);
    }
}

__pid_t fork(void) {
    if (!scaler::fork_orig) {
        scaler::fork_orig = (scaler::fork_origt) dlsym(RTLD_NEXT, "fork");
        fatalError("Cannot find the address of fork_orig");
        return false;
    }


    assert(scaler::fork_orig != nullptr);
    __pid_t result = (*scaler::fork_orig)();
    if (result == 0) {
        //Child process
        //Clear recording buffer


    }
    return result;
}


//void pthread_exit(void *__retval) {
//    saveRecordBuffer();
//    scaler::pthread_exit_orig(__retval);
//}



}

#endif
