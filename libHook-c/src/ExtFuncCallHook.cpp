#ifdef __linux

#include <util/hook/ExtFuncCallHook.h>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/ElfParser.h>
#include <exceptions/ScalerException.h>
#include <sys/mman.h>
#include <cassert>
#include <elf.h>
#include <set>
#include <utility>
#include <util/tool/MemoryTool.h>
#include <util/tool/Math.h>
#include <type/ELFImgInfo.h>
#include <util/hook/HookHandlers.h>
#include <type/ELFSecInfo.h>
#include <util/hook/HookContext.h>
#include <util/tool/StringTool.h>
#include <util/tool/Timer.h>
#include <util/hook/LogicalClock.h>
#include "util/hook/proxy/DLProxy.h"


namespace scaler {


    bool ExtFuncCallHook::install() {
        createRecordingFolder();

        std::stringstream ss;
        ss << scaler::ExtFuncCallHook::instance->folderName << "/symbolInfo.txt";
        FILE *symInfoFile = fopen(ss.str().c_str(), "a");
        if (!symInfoFile) {
            fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }
        fprintf(symInfoFile, "%s,%s,%s\n", "funcName", "fileId", "symIdInFile");
        fclose(symInfoFile);

        parseRequiredInfo(0);

        if (!initTLS()) {
            ERR_LOG("Failed to initialize TLS");
            //This is the main thread
        }

        DBG_LOG("Initialize logical clock");
        initLogicalClock(curContext->cachedWallClockSnapshot, curContext->cachedLogicalClock,
                         curContext->cachedThreadNum);

        DBG_LOG("Replace PLT entry");
        replacePltEntry(0);

        return true;
    }

    bool ExtFuncCallHook::uninstall() {
//        //todo: release oriPltCode oriPltSecCode
//        //Decallocate recordbuffer for main thread
//
//        for (SymID curSymId: hookedExtSymbol) {
//            auto &curSymbol = allExtSymbol[curSymId];
//
//            //DBG_LOGS("[%s] %s hooked (ID:%zd)\n", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
//            //curSymbol.extSymbolId);
//            void *oriCodePtr = nullptr;
//
//            if (curSymbol.oriPltSecCode != nullptr) {
//                //.plt.sec table exists
//                //todo: adjust the permission back after this
//                if (!adjustMemPerm(
//                        (uint8_t *) curSymbol.pltSecEntry,
//                        (uint8_t *) curSymbol.pltSecEntry + 16,
//                        PROT_READ | PROT_WRITE | PROT_EXEC)) {
//                    ERR_LOG("Cannot adjust memory permission");
//                    continue;
//                }
//                memcpy((uint8_t *) curSymbol.pltSecEntry,
//                       curSymbol.oriPltSecCode, 16);
//                free(curSymbol.oriPltSecCode);
//                curSymbol.oriPltSecCode = nullptr;
//
//            }
//
//            if (curSymbol.oriPltCode != nullptr) {
//                //todo: what is this doesn't exist (for example, installer failed at this symbol)
//                if (!adjustMemPerm(
//                        (uint8_t *) curSymbol.pltEntry,
//                        (uint8_t *) curSymbol.pltEntry + 16,
//                        PROT_READ | PROT_WRITE | PROT_EXEC)) {
//                    ERR_LOG("Cannot adjust memory permission");
//                    continue;
//                }
//                memcpy((uint8_t *) curSymbol.pltEntry,
//                       curSymbol.oriPltCode, 16);
//                free(curSymbol.oriPltCode);
//                curSymbol.oriPltCode = nullptr;
//            }
//        }
//        installed = false;
        return true;
    }

    ExtFuncCallHook *ExtFuncCallHook::instance = nullptr;

    ExtFuncCallHook *ExtFuncCallHook::getInst(std::string folderName) {
        if (!instance) {
            instance = new ExtFuncCallHook(std::move(folderName));
            if (!instance) {
                fatalError("Cannot allocate memory for ExtFuncCallHookAsm");
                return nullptr;
            }
        }
        return instance;
    }

    ExtFuncCallHook *ExtFuncCallHook::getInst() {
        if (!instance) {
            fatalError(
                    "Cannot create object using  this function. Users can use getInst(std::string folderName) first and then call this function");
            return nullptr;
        }
        return instance;
    }

    ExtFuncCallHook::~ExtFuncCallHook() {
        //todo: release oriPltCode oriPltSecCode
        uninstall();
        pthread_mutex_destroy(&dynamicLoadingLock);
    }

    ExtFuncCallHook::ExtFuncCallHook(std::string folderName) : folderName(folderName), pmParser(folderName),
                                                               elfImgInfoMap(1024), allExtSymbol(1024) {
        pthread_mutex_t mutex;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    bool ExtFuncCallHook::makeGOTWritable(ELFSecInfo &gotSec, bool writable) {
        if (writable) {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);
        } else {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ);
        }
    }

    bool ExtFuncCallHook::parseSymbolInfo(ssize_t loadingId, ELFParser &parser, ssize_t fileId, uint8_t *baseAddr,
                                          ELFSecInfo &pltSection,
                                          ELFSecInfo &pltSecureSection, ELFSecInfo &gotSec, uint8_t *startAddr,
                                          uint8_t *endAddr) {

        //assert(sizeof(ExtSymInfo) % 32 == 0); //Force memory allignment
        //INFO_LOGS("sizeof(ExtSymInfo)=%d", a);

        ELFImgInfo &curImgInfo = elfImgInfoMap[loadingId][fileId];
        curImgInfo.firstSymIndex = allExtSymbol[loadingId].getSize();
        //Allocate space for all rela entries in this file
        //DBG_LOGS("First sym index=%ld", curImgInfo.firstSymIndex);

        adjustMemPerm(pltSection.startAddr, pltSection.startAddr + pltSection.size, PROT_READ | PROT_WRITE | PROT_EXEC);
        adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);

        if (pltSecureSection.startAddr) {
//            DBG_LOGS("Adjusting mem permission from:%p to:%p", pltSecureSection.startAddr,
//                     pltSecureSection.startAddr + pltSecureSection.size);
            adjustMemPerm(pltSecureSection.startAddr, pltSecureSection.startAddr + pltSecureSection.size,
                          PROT_READ | PROT_WRITE | PROT_EXEC);
        }
        std::stringstream ss;
        ss << scaler::ExtFuncCallHook::instance->folderName << "/symbolInfo.txt";
        FILE *symInfoFile = fopen(ss.str().c_str(), "a");
        if (!symInfoFile) {
            fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }
        assert(pltSection.size / pltSection.entrySize == parser.relaEntrySize + 1);
        for (ssize_t i = 0; i < parser.relaEntrySize; ++i) {
            const char *funcName;
            Elf64_Word type;
            Elf64_Word bind;
            parser.getExtSymbolInfo(i, funcName, bind, type);
            ssize_t initialGap = 0;

            void *addressOverride = nullptr;
            if (!shouldHookThisSymbol(funcName, bind, type, allExtSymbol[loadingId].getSize(), initialGap,
                                      addressOverride)) {
                continue;
            }
            //Get function id from plt entry

            uint8_t **gotAddr = (uint8_t **) autoAddBaseAddr((uint8_t *) (parser.getRelaOffset(i)), baseAddr, startAddr,
                                                             endAddr);
            uint8_t *curGotDest = *gotAddr;
            uint8_t *pltSecEntry = nullptr;

            if (curImgInfo.pltSecStartAddr) {
                pltSecEntry = curImgInfo.pltSecStartAddr + pltSecureSection.entrySize * i;
            }
            uint8_t *pltEntry = curImgInfo.pltStartAddr + pltSection.entrySize * (i + 1);

            //DBG_LOGS("curImgInfo.pltStartAddr = %p\n", curImgInfo.pltStartAddr);
            uint32_t pltStubId = parsePltStubId(pltEntry); //Note that the first entry is not valid

            //Make sure space is enough, if space is enough, array won't allocate
            ExtSymInfo *newSym = allExtSymbol[loadingId].pushBack();

//            newSym->addrResolved = abs(curGotDest - pltSection.startAddr) > pltSection.size;
            newSym->fileId = fileId;
            newSym->symIdInFile = i;
            newSym->gotEntryAddr = gotAddr;
            newSym->pltEntryAddr = pltEntry;
            newSym->pltSecEntryAddr = pltSecEntry;
            newSym->pltStubId = pltStubId;
            newSym->initialGap = initialGap;
            if (addressOverride) {
                INFO_LOGS("%p", gotAddr);
                *newSym->gotEntryAddr = reinterpret_cast<uint8_t *>(addressOverride);
            }
            fprintf(symInfoFile, "%s,%ld,%ld\n", funcName, newSym->fileId, newSym->symIdInFile);

            DBG_LOGS(
                    "id:%ld funcName:%s gotAddr:%p *gotAddr:%p fileId:%zd symIdInFile:%zd pltEntryAddr:%p pltSecEntryAddr:%p pltStubId:%lu\n",
                    allExtSymbol[loadingId].getSize() - 1, funcName, gotAddr, *gotAddr,
                    fileId,
                    newSym->symIdInFile, newSym->pltEntryAddr, newSym->pltSecEntryAddr, newSym->pltStubId);
        }

        fclose(symInfoFile);
        return true;
    }


    const int SAMPLING_GAP = 0b0;

    bool ExtFuncCallHook::shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type, SymID curSymId,
                                               ssize_t &initialGap, void *&addressOverride) {
        addressOverride = nullptr;
        initialGap = 0;
        if (bind != STB_GLOBAL || type != STT_FUNC) {
            return false;
        }

        ssize_t funcNameLen = strlen(funcName);
        if (funcNameLen == 0) {
            return false;
//            fatalError("Function has no name?!");
        }


        if (scaler::strStartsWith(funcName, "__")) {
            return false;
        }

        if (funcNameLen == 3) {
            if (strncmp(funcName, "cos", 3) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "exp", 3) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "log", 3) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "sin", 3) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "oom", 3) == 0) {
                return false;
            } else if (strncmp(funcName, "err", 3) == 0) {
                return false;
            }
        } else if (funcNameLen == 4) {
            if (strncmp(funcName, "cosf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "expf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "logf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "powf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "sinf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "sqrtf", 4) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "jump", 4) == 0) {
                return false;
            } else if (strncmp(funcName, "exit", 4) == 0) {
                return false;
            } else if (strncmp(funcName, "fail", 4) == 0) {
                return false;
            } else if (strncmp(funcName, "verr", 4) == 0) {
                return false;
            } else if (strncmp(funcName, "errx", 4) == 0) {
                return false;
            }
        } else if (funcNameLen == 5) {
            if (strncmp(funcName, "atan2", 5) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "sqrtf", 5) == 0) {
                initialGap = SAMPLING_GAP;
            } else if (strncmp(funcName, "_exit", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "abort", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "_Exit", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "verrx", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "dlsym", 5) == 0) {
                INFO_LOG("dlsym overrided");
//                addressOverride = (void *) dlsym_proxy;
                return false;
            }
        } else if (funcNameLen == 6) {
            if (strncmp(funcName, "_ZdlPv", 6) == 0) {
                return false;
            } else if (strncmp(funcName, "dlopen", 6) == 0) {
                INFO_LOG("dlopen address overrided");
//                addressOverride = (void *) dlopen_proxy;
                return false;
            }
        } else if (funcNameLen == 7) {
            if (strncmp(funcName, "_dl_sym", 7) == 0) {
                return false;
            } else if (strncmp(funcName, "longjmp", 7) == 0) {
                return false;
            } else if (strncmp(funcName, "_setjmp", 7) == 0) {
                return false;
            }


        } else if (funcNameLen == 8) {
            if (strncmp(funcName, "_longjmp", 8) == 0) {
                return false;
            } else if (strncmp(funcName, "__assert", 8) == 0) {
                return false;
            }
        } else if (funcNameLen == 9) {
            if (strncmp(funcName, "thrd_exit", 9) == 0) {
                return false;
            } else if (strncmp(funcName, "__longjmp", 9) == 0) {
                return false;
            }
        } else if (funcNameLen == 10) {
            if (strncmp(funcName, "siglongjmp", 10) == 0) {
                return false;
            } else if (strncmp(funcName, "quick_exit", 10) == 0) {
                return false;
            } else if (strncmp(funcName, "__chk_fail", 10) == 0) {
                return false;
            } else if (strncmp(funcName, "__REDIRECT", 10) == 0) {
                return false;
            }
        } else if (funcNameLen == 11) {
            if (strncmp(funcName, "__sigsetjmp", 11) == 0) {
                return false;
            } else if (strncmp(funcName, "__do_cancel", 11) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_throw", 11) == 0) {
                return false;
            }
        } else if (funcNameLen == 12) {
            if (strncmp(funcName, "pthread_exit", 12) == 0) {
                return false;
            } else if (strncmp(funcName, "__libc_fatal", 12) == 0) {
                return false;
            }
        } else if (funcNameLen == 13) {
            if (strncmp(funcName, "__longjmp_chk", 13) == 0) {
                return false;
            } else if (strncmp(funcName, "__assert_fail", 13) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_rethrow", 13) == 0) {
                return false;
            }
        } else if (funcNameLen == 14) {
            if (strncmp(funcName, "__tls_get_addr", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__pthread_exit", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "_startup_fatal", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__ia64_longjmp", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__libc_longjmp", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmxlongjmp", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "nscd_run_prune", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "main_loop_poll", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__libc_longjmp", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__libc_message", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_bad_cast", 14) == 0) {
                return false;
            } else if (strncmp(funcName, "pthread_create", 14) == 0) {
                //pthreadCreateSymId = curSymId;
                //todo: Also calculate the time of pthread_create
                //todo: Temporary measure. Make sure pthread_create address is correctly resolved
                //This is important to make sure pthread_create is recorded
                return false;
            }
        } else if (funcNameLen == 15) {
            if (strncmp(funcName, "____longjmp_chk", 15) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmx_longjmp", 15) == 0) {
                return false;
            } else if (strncmp(funcName, "nscd_run_worker", 15) == 0) {
                return false;
            } else if (strncmp(funcName, "____longjmp_chk", 15) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_catch_error", 15) == 0) {
                return false;
            }
        } else if (funcNameLen == 16) {
            if (strncmp(funcName, "__REDIRECT_NTHNL", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "__pthread_unwind", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_fatal_printf", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_signal_error", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "__longjmp_cancel", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmx__longjmp", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_allocate_tls", 16) == 0) {
                return false;
            } else if (strncmp(funcName, "__call_tls_dtors", 16) == 0) {
                return false;
            }
        } else if (funcNameLen == 17) {
            if (strncmp(funcName, "__tunable_get_val", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "futex_fatal_error", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmxsiglongjmp", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "__libc_siglongjmp", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "libc_hidden_proto", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "rtld_hidden_proto", 17) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_begin_catch", 17) == 0) {
                return false;
            }
        } else if (funcNameLen == 18) {
            if (strncmp(funcName, "_dl_reloc_bad_type", 18) == 0) {
                return false;
            } else if (strncmp(funcName, "__assert_fail_base", 18) == 0) {
                return false;
            }
        } else if (funcNameLen == 19) {
            if (strncmp(funcName, "termination_handler", 19) == 0) {
                return false;
            } else if (strncmp(funcName, "receive_print_stats", 19) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_catch_exception", 19) == 0) {
                return false;
            }
        } else if (funcNameLen == 20) {
            if (strncmp(funcName, "_dl_signal_exception", 20) == 0) {
                return false;
            } else if (strncmp(funcName, "__assert_perror_fail", 20) == 0) {
                return false;
            } else if (strncmp(funcName, "_ZSt13get_terminatev", 20) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_free_exception", 20) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_exception_create", 20) == 0) {
                return false;
            }
        } else if (funcNameLen == 21) {
            if (strncmp(funcName, "__pthread_unwind_next", 21) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmx__libc_longjmp", 21) == 0) {
                return false;
            } else if (strncmp(funcName, "_dl_allocate_tls_init", 21) == 0) {
                return false;
            }
        } else if (funcNameLen == 22) {
            if (strncmp(funcName, "_Unwind_RaiseException", 22) == 0) {
                return false;
            }
        } else if (funcNameLen == 23) {
            if (strncmp(funcName, "_dl_find_dso_for_object", 23) == 0) {
                return false;
            } else if (strncmp(funcName, "svctcp_rendezvous_abort", 23) == 0) {
                return false;
            } else if (strncmp(funcName, "_Unwind_DeleteException", 23) == 0) {
                return false;
            }
        } else if (funcNameLen == 24) {
            if (strncmp(funcName, "svcunix_rendezvous_abort", 24) == 0) {
                return false;
            } else if (strncmp(funcName, "__novmx__libc_siglongjmp", 24) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_allocate_exception", 24) == 0) {
                return false;
            }
        } else if (funcNameLen == 28) {
            if (strncmp(funcName, "__cxa_init_primary_exception", 28) == 0) {
                return false;
            } else if (strncmp(funcName, "__cxa_current_exception_type", 28) == 0) {
                return false;
            }
        } else if (funcNameLen == 30) {
            if (strncmp(funcName, "__cxa_free_dependent_exception", 30) == 0) {
                return false;
            }
        } else if (funcNameLen == 34) {
            if (strncmp(funcName, "__cxa_allocate_dependent_exception", 34) == 0) {
                return false;
            }
        }
        return true;
    }


    bool ExtFuncCallHook::parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltInfo, ELFSecInfo &pltSecInfo,
                                        ELFSecInfo &gotInfo,
                                        uint8_t *baseAddr, uint8_t *startAddr, uint8_t *endAddr) {
        Elf64_Shdr pltHdr;
        if (!elfParser.getSecHeader(SHT_PROGBITS, ".plt", pltHdr)) {
            ERR_LOG("Cannot read .plt header");
            return false;
        }
        pltInfo.size = pltHdr.sh_size;
        pltInfo.entrySize = pltHdr.sh_entsize;

        Elf64_Shdr pltSecHdr;
        pltSecInfo.entrySize = 0;
        if (elfParser.getSecHeader(SHT_PROGBITS, ".plt.sec", pltSecHdr)) {
            pltSecInfo.size = pltSecHdr.sh_size;
            pltSecInfo.entrySize = pltSecHdr.sh_entsize;
        }


        Elf64_Shdr gotHdr;
        if (!elfParser.getSecHeader(SHT_PROGBITS, ".got", gotHdr)) {
            ERR_LOG("Cannot read .got header");
            return false;
        }
        gotInfo.size = gotHdr.sh_size;
        gotInfo.entrySize = gotHdr.sh_entsize;


        pltInfo.startAddr = autoAddBaseAddr((uint8_t *) pltHdr.sh_addr, baseAddr, startAddr, endAddr);
        gotInfo.startAddr = autoAddBaseAddr((uint8_t *) gotHdr.sh_addr, baseAddr, startAddr, endAddr);

        if (pltSecInfo.entrySize > 0) {
            //Have .plt.sec table
            pltSecInfo.startAddr = autoAddBaseAddr((uint8_t *) pltSecHdr.sh_addr, baseAddr, startAddr, endAddr);
        } else {
            pltSecInfo.startAddr = nullptr;
        }

        return pltInfo.startAddr != nullptr && gotInfo.startAddr != nullptr;
    }

    //16bytes aligned. 0x90 are for alignment purpose
    uint8_t pltEntryBin[] = {0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x41, 0xff, 0xE3, 0x90, 0x90, 0x90};
    //32bytes aligned. 0x90 are for alignment purpose

    const int READ_TLS_PART_START = 0;
    const int COUNT_TLS_ARR_ADDR = READ_TLS_PART_START + 2;

    const int COUNTING_PART_START = READ_TLS_PART_START + 20;
    const int LD_ARRAY_OFFSET1 = COUNTING_PART_START + 12, DYMAIC_LOADING_OFFSET1_SIZE = 32;
    const int COUNT_OFFSET1 = COUNTING_PART_START + 19, COUNT_OFFSET1_SIZE = 32;
    const int COUNT_OFFSET2 = COUNTING_PART_START + 30, COUNT_OFFSET2_SIZE = 32;
    const int GAP_OFFSET = COUNTING_PART_START + 37, GAP_OFFSET_SIZE = 32;

    const int SKIP_PART_START = COUNTING_PART_START + 52;
    const int GOT_ADDR = SKIP_PART_START + 2, GOT_ADDR_SIZE = 64;
    const int CALL_LD_INST = SKIP_PART_START + 13;
    const int PLT_STUB_ID = SKIP_PART_START + 14, PLT_STUB_ID_SIZE = 32;
    const int PLT_START_ADDR = SKIP_PART_START + 20, PLT_START_ADDR_SIZE = 64;

    const int INTERNALARR_OFFSET_IN_ARR = 0x8;
    const int COUNT_OFFSET_IN_RECARR = 0x10;
    const int GAP_OFFSET_IN_RECARR = 0x18;

    const int TIMING_PART_START = SKIP_PART_START + 31;
    const int LOADING_ID = TIMING_PART_START + 1, LOADING_ID_SIZE = 32;
    const int SYM_ID = TIMING_PART_START + 6, FUNC_ID_SIZE = 32;
    const int ASM_HOOK_HANDLER_ADDR = TIMING_PART_START + 12, ASM_HOOK_HANDLER_ADDR_SIZE = 64;

    uint8_t idSaverBin[] = {
            /**
             * Read TLS part
             */
            //mov $0x1122334455667788,%r11 | move TLS offset to r11
            0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //mov %fs:(%r11),%r11 | move TLS address to r11
            0x64, 0x4D, 0x8B, 0x1B,
            //cmpq $0,%r11 | Check TLS is initialized or not
            0x49, 0x83, 0xFB, 0x00,
            //je SKIP | If TLS initialized, do not invoke any hook
            0x74, 0x2D,

            /**
             * Counting part
             */
            //push %r10 | Save register r10
            0x41, 0x52,
            //mov    0x828(%funcr11),%r11 | mov the value of Context.ldArr to  r11
            0x4D, 0x8B, 0x9B, 0x28, 0x08, 0x00, 0x00,
            //mov    0x00000000(%r11),%r11 | move ldArr.internalArr[loadingId].internalArr address to r11
            0x4D, 0x8B, 0x9B, 0x00, 0x00, 0x00, 0x00,
            //mov    0x00000000(%r11),%r10 | move the value of current API's invocation counter to r10
            0x4D, 0x8B, 0x93, 0x00, 0x00, 0x00, 0x00,
            //add    $0x1,%r10 | Increase counter by 1
            0x49, 0x83, 0xC2, 0x01,
            //mov    %r10,0x11223344(%r11) | Store updated counter back
            0x4D, 0x89, 0x93, 0x00, 0x00, 0x00, 0x00,
            //mov    0x11223344(%r11),%r11 | move the value of current API's gap to r10
            0x4D, 0x8B, 0x9B, 0x00, 0x00, 0x00, 0x00,
            //and    %r11,%r10 | counter % gap
            0x4D, 0x21, 0xDA,
            //cmpq   $0x0,%r10 | Check the following if
            0x49, 0x83, 0xFA, 0x00,
            //pop    %r10 | Restore the value of r10
            0x41, 0x5A,
            //jz TIMING_JUMPER | If counter % gap == 0 skip, otherwise jump to TIMING part
            0x74, 0x1F,

            /**
             * SKIP part
             */
            //SKIP:
            //movq $0x1122334455667788,%r11 | Move current API's GOT address to Scaler's context
            0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //jmpq (%r11) | Jump to the address of API's GOT entry
            0x41, 0xFF, 0x23,

            //pushq $0x11223344 | Save the ld.so id of this API to the stack. This value will be picked up by ld.so.
            0x68, 0x00, 0x00, 0x00, 0x00,
            //movq $0x1122334455667788,%r11 | move the address of PLT[0] to r11
            0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //jmpq *%r11 | Jump to PLT[0]
            0x41, 0xFF, 0xE3,

            /**
             * TIMING part
             */
            //pushq $0x11223344 | Save the loading id of this API to the stack. This value will be picked up by asmHookHandler
            0x68, 0x00, 0x00, 0x00, 0x00,
            //pushq $0x11223344 | Save the scaler id of this API to the stack.  This value will be picked up by asmHookHandler
            0x68, 0x00, 0x00, 0x00, 0x00,
            //movq $0x1122334455667788,%r11 | Move the address of asmHookHandler to r11
            0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //jmpq *%r11 | Jump to asmHookHandler
            0x41, 0xFF, 0xE3
    };

    uint32_t ExtFuncCallHook::parsePltStubId(uint8_t *dest) {
        int pushOffset = -1;
        if (*dest == 0xFF) {
            pushOffset = 7;
        } else if (*dest == 0xF3) {
            pushOffset = 5;
        } else {
            fatalError("Plt entry format illegal. Cannot find instruction \"push id\"");
        }

        //Debug tips: Add breakpoint after this statement, and *pltStubId should be 0 at first, 2 at second .etc
        uint32_t *pltStubId = reinterpret_cast<uint32_t *>(dest + pushOffset);
        return *pltStubId;
    }

    bool ExtFuncCallHook::fillAddr2pltEntry(uint8_t *funcAddr, uint8_t *retPltEntry) {
        //Copy code
        memcpy(retPltEntry, pltEntryBin, sizeof(pltEntryBin));
        //Copy address
        assert(sizeof(uint8_t **) == 8);
        memcpy(retPltEntry + 2, &funcAddr, sizeof(uint8_t **));
        return true;
    }

    bool ExtFuncCallHook::fillAddrAndSymId2IdSaver(uint8_t **gotAddr, uint8_t *firstPltEntry, uint32_t symId,
                                                   uint32_t pltStubId, uint32_t ldArrayOffset, uint32_t loadingId,
                                                   uint32_t countOffset, uint32_t gapOffset, uint8_t *idSaverEntry) {

        //Copy code
        memcpy(idSaverEntry, idSaverBin, sizeof(idSaverBin));

        assert(sizeof(uint8_t **) == 8);

        uint8_t *tlsOffset = nullptr;
        __asm__ __volatile__ (
                "movq 0x2FAC7B(%%rip),%0\n\t"
                :"=r" (tlsOffset)
                :
                :
                );

        memcpy(idSaverEntry + COUNT_TLS_ARR_ADDR, &tlsOffset, sizeof(void *));

        //Fill TLS offset (Address filled directly inside)
        memcpy(idSaverEntry + LD_ARRAY_OFFSET1, &ldArrayOffset, sizeof(uint32_t));
        memcpy(idSaverEntry + COUNT_OFFSET1, &countOffset, sizeof(uint32_t));
        memcpy(idSaverEntry + COUNT_OFFSET2, &countOffset, sizeof(uint32_t));

        memcpy(idSaverEntry + GAP_OFFSET, &gapOffset, sizeof(uint32_t));

        //Fill got address
        memcpy(idSaverEntry + GOT_ADDR, &gotAddr, sizeof(uint8_t **));
        //Fill function id
        memcpy(idSaverEntry + PLT_STUB_ID, &pltStubId, sizeof(uint32_t));
        //Fill first plt address
        memcpy(idSaverEntry + PLT_START_ADDR, &firstPltEntry, sizeof(uint8_t *));

        //Fill loadingId
        memcpy(idSaverEntry + LOADING_ID, &loadingId, sizeof(uint32_t));
        //Fill symId
        memcpy(idSaverEntry + SYM_ID, &symId, sizeof(uint32_t));


        uint8_t *asmHookPtr = (uint8_t *) &asmTimingHandler;
        //Fill asmTimingHandler
        memcpy(idSaverEntry + ASM_HOOK_HANDLER_ADDR, (void *) &asmHookPtr, sizeof(void *));

        return true;
    }


    void ExtFuncCallHook::parseRequiredInfo(ssize_t loadingId) {
        pthread_mutex_lock(&dynamicLoadingLock);
        //Initialize existing loading id

        ELFParser elfParser;
        if (!pmParser.parsePMMap(loadingId)) {
            fatalErrorS("Cannot parsePmMap in loding %zd", loadingId);
        }
        pmParser.acruieReadLock();
        //Find new file from exising PMMaps
        Array<FileID> newFileEntryId;
        pmParser.getNewFileEntryIdsUnsafe(loadingId, newFileEntryId, true);
        elfImgInfoMap.pushBack();
        allExtSymbol.pushBack();
        assert(elfImgInfoMap.getSize() - 1 == loadingId);
        assert(allExtSymbol.getSize() - 1 == loadingId);

        //elfImgInfoMap is always incremental, allocate room for newly allocated files
        ssize_t elemDifference = pmParser.getFileEntryArraySize() - elfImgInfoMap[loadingId].getSize();
        for (int i = 0; i < elemDifference; ++i) {
            ELFImgInfo *curElfImgInfo = elfImgInfoMap[loadingId].pushBack();
            curElfImgInfo->valid = false; //Set them to invalid by default
        }
        //Get segment info from /proc/self/maps
        for (ssize_t i = 0; i < newFileEntryId.getSize(); ++i) {
            FileID fileId = newFileEntryId[i];
            INFO_LOGS("fileId=%zd", fileId);
            FileEntry &curFileEntry = pmParser.getFileEntryUnSafe(fileId);
            const char *curFilePathName = pmParser.getStrUnsafe(curFileEntry.pathNameStartIndex);
            INFO_LOGS("%s", curFilePathName);
            ELFImgInfo &curElfImgInfo = elfImgInfoMap[loadingId][fileId];
            if (elfParser.parse(curFilePathName)) {
                //Find the entry size of plt and got
                ELFSecInfo pltInfo{};
                ELFSecInfo pltSecInfo{};
                ELFSecInfo gotInfo{};

                //todo: We assume plt and got entry size is the same.
                if (!parseSecInfos(elfParser, pltInfo, pltSecInfo, gotInfo, curFileEntry.baseStartAddr,
                                   curFileEntry.baseStartAddr, curFileEntry.baseEndAddr)) {
                    fatalError("Failed to parse plt related sections.");
                    exit(-1);
                }
                curElfImgInfo.pltStartAddr = pltInfo.startAddr;
                curElfImgInfo.pltSecStartAddr = pltSecInfo.startAddr;
                curElfImgInfo.gotStartAddr = gotInfo.startAddr;

                //ERR_LOGS("%zd:%s %p pltStartAddr=%p", fileId, pmParser.getStrUnsafe(curFileEntry.pathNameStartIndex),
                //         curFileEntry.baseStartAddr, pltInfo.startAddr);

                //Install hook on this file
                if (!parseSymbolInfo(loadingId, elfParser, fileId, curFileEntry.baseStartAddr, pltInfo, pltSecInfo,
                                     gotInfo, curFileEntry.baseStartAddr,
                                     curFileEntry.baseEndAddr)) {
                    fatalErrorS("installation for file %s failed.", curFilePathName);
                    exit(-1);
                }
                curElfImgInfo.valid = true;
            }
        }
        pmParser.releaseReadLock();
        pthread_mutex_unlock(&dynamicLoadingLock);
    }

    bool ExtFuncCallHook::replacePltEntry(ssize_t loadingId) {
        //Allocate callIdSaver
        //todo: memory leak
        callIdSavers = static_cast<uint8_t *>(mmap(NULL, allExtSymbol[loadingId].getSize() * sizeof(idSaverBin),
                                                   PROT_READ | PROT_WRITE,
                                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        /**
         * Prepare callIdSaver
         */
        adjustMemPerm(callIdSavers, callIdSavers + allExtSymbol[loadingId].getSize() * sizeof(idSaverBin),
                      PROT_READ | PROT_WRITE | PROT_EXEC);
        uint8_t *curCallIdSaver = callIdSavers;
        //Fill address and ids in callIdSaver
        for (int curSymId = 0; curSymId < allExtSymbol[loadingId].getSize(); ++curSymId) {
            ExtSymInfo &curSymInfo = allExtSymbol[loadingId][curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[loadingId][curSymInfo.fileId];

            if (!fillAddrAndSymId2IdSaver(curSymInfo.gotEntryAddr, curImgInfo.pltStartAddr, curSymId,
                                          curSymInfo.pltStubId,
                                          loadingId * sizeof(scaler::Array<RecTuple>) + INTERNALARR_OFFSET_IN_ARR,
                                          loadingId,
                                          curSymId * sizeof(RecTuple) + COUNT_OFFSET_IN_RECARR,
                                          curSymId * sizeof(RecTuple) + GAP_OFFSET_IN_RECARR,
                                          curCallIdSaver)) {
                fatalError("fillAddrAndSymId2IdSaver failed, this should not happen");
            }
            curCallIdSaver += sizeof(idSaverBin);
        }

        /**
         * Replace plt entry or replace .plt (Or directly replace .plt.sec)
         */
        curCallIdSaver = callIdSavers;
        for (int curSymId = 0; curSymId < allExtSymbol[loadingId].getSize(); ++curSymId) {
            ExtSymInfo &curSym = allExtSymbol[loadingId][curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[loadingId][curSym.fileId];

            if (curSym.pltSecEntryAddr) {
                //Replace .plt.sec
                if (!fillAddr2pltEntry(curCallIdSaver, curSym.pltSecEntryAddr)) {
                    fatalError("pltSecAddr installation failed, this should not happen");
                }
            } else {
                //Replace .plt
                if (!fillAddr2pltEntry(curCallIdSaver, curSym.pltEntryAddr)) {
                    fatalError("pltEntry installation failed, this should not happen");
                }
            }
            //Replace got entry, 16 is the size of a plt entry
            if (abs(curSym.pltEntryAddr - *curSym.gotEntryAddr) < 16) {
                //Address not resolved
                *curSym.gotEntryAddr = curCallIdSaver + CALL_LD_INST;
            }

            curCallIdSaver += sizeof(idSaverBin);

        }

        return true;
    }

    void ExtFuncCallHook::createRecordingFolder() const {
        //sprintf(folderName, "scalerdata_%lu", getunixtimestampms());
        if (mkdir(folderName.c_str(), 0755) == -1) {
            fatalErrorS("Cannot mkdir ./%s because: %s", folderName.c_str(),
                        strerror(errno));
        }

    }


}

#endif
