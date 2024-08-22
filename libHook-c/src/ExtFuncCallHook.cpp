/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifdef __linux

#include <util/hook/ExtFuncCallHook.h>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/ElfParser.h>
#include <exceptions/ScalerException.h>
#include <sys/mman.h>
#include <cassert>
#include <elf.h>
#include <utility>
#include <util/tool/MemoryTool.h>
#include <type/ELFImgInfo.h>
#include <util/hook/HookHandlers.h>
#include <type/ELFSecInfo.h>
#include <util/hook/HookContext.h>
#include <util/tool/StringTool.h>
#include "util/hook/proxy/DLProxy.h"
#include "util/hook/proxy/SystemProxy.h"
#include <util/hook/proxy/PthreadProxy.h>

namespace scaler {
    bool installed = false;

    bool ExtFuncCallHook::install() {
        createRecordingFolder();

        if (!initTLS()) {
            ERR_LOG("Failed to initialize TLS");
            //This is the main thread
            return false;
        }
        return installDlOpen();
    }


    bool ExtFuncCallHook::installDlOpen() {
        INFO_LOG("Install DlOpen");
        parseRequiredInfo();

        populateRecordingArray(*this, curContext); //Invoke this again because new loadingID is loaded
        //DBG_LOG("Replace PLT entry");

        //HookContext *curContextPtr = curContext;
        //DBG_LOGS("ContextPtr=%p",curContextPtr);
        //DBG_LOGS("&ContextPtr.recordingArray=%p",&curContextPtr->recordingArray[0].internalArr);
        //DBG_LOGS("ContextPtr.recordingArray=%p",&curContextPtr->recordingArray[0].internalArr[2].count);

        return replacePltEntry();
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
            if (!instance) { fatalError("Cannot allocate memory for ExtFuncCallHookAsm");
                return nullptr;
            }
        }
        return instance;
    }

    ExtFuncCallHook *ExtFuncCallHook::getInst() {
        if (!instance) { fatalError(
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
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&dynamicLoadingLock, &attr);
    }


    bool ExtFuncCallHook::makeGOTWritable(ELFSecInfo &gotSec, bool writable) {
        if (writable) {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);
        } else {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ);
        }
    }

    inline bool ExtFuncCallHook::parseRela(ssize_t &validRelaEntrySize, const SymbolType &symbolType, ELFParser &parser,
                                           ELFImgInfo &curImgInfo, FileID fileId, ELFSecInfo &pltSection,
                                           ELFSecInfo &pltSecureSection, Elf64_Rela *&relaSection,
                                           ssize_t relaEntrySize,
                                           FILE *symInfoFile, uint8_t *baseAddr, uint8_t *startAddr, uint8_t *endAddr) {
        for (ssize_t i = 0; i < relaEntrySize; ++i) {
            const char *funcName;
            Elf64_Word type;
            Elf64_Word bind;
            parser.getExtSymbolInfo(i, funcName, bind, type, relaSection);
            ssize_t initialGap = 0;

            void *addressOverride = nullptr;

            if (!shouldHookThisSymbol(funcName, bind, type, initialGap, addressOverride)) {
                //INFO_LOGS("shouldNotHookThisSymbol? fundId=%ld funcName=%s",allExtSymbol.getSize(), funcName);
                continue;
            }
            uint8_t **gotAddr = (uint8_t **) autoAddBaseAddr((uint8_t *) (parser.getRelaOffset(i, relaSection)),
                                                             baseAddr, startAddr, endAddr);
            auto pmParserIter = pmParser.findPmEntryIdByAddr(*gotAddr);
            if (!pmParserIter->isE()) {
                continue;
            }
            //INFO_LOGS("shouldHookThisSymbol? fundId=%ld funcName=%s type=%d", allExtSymbol.getSize(), funcName, type);

            uint8_t *curGotDest = *gotAddr;
            ssize_t symbolNumber = allExtSymbol.getSize();

            ExtSymInfo *newSym = allExtSymbol.pushBack();
            newSym->symbolType = symbolType;
            newSym->fileId = fileId;
            newSym->symIdInFile = i;
            newSym->initialGap = initialGap;
            newSym->addressOverride = addressOverride;

            if (symbolType == SymbolType::C_PLT_API) {
                uint8_t *pltSecEntry = nullptr;
                if (curImgInfo.pltSecStartAddr) {
                    pltSecEntry = curImgInfo.pltSecStartAddr + pltSecureSection.entrySize * i;
                }
                uint8_t *pltEntry = curImgInfo.pltStartAddr + pltSection.entrySize * (i + 1);

                ssize_t pltStubId = parsePltStubId(pltEntry);

                newSym->realAddrPtr = gotAddr;
                newSym->gotEntryAddr = gotAddr;
                newSym->pltEntryAddr = pltEntry;
                newSym->pltSecEntryAddr = pltSecEntry;
                newSym->pltStubId = pltStubId;

            } else if (symbolType == SymbolType::C_DYN_API) {
                newSym->gotEntryAddr = gotAddr;
            }

            if(symInfoFile){
                fprintf(symInfoFile, "%s,%ld,%ld\n", funcName, newSym->fileId, newSym->symIdInFile);
            }

            validRelaEntrySize += 1;

        }
        return true;
    }

    bool ExtFuncCallHook::parseSymbolInfo(ELFParser &parser, ssize_t fileId, FileEntry &fileEntry,
                                          ELFSecInfo &pltSection,
                                          ELFSecInfo &pltSecureSection, ELFSecInfo &gotSec) {

        //assert(sizeof(ExtSymInfo) % 32 == 0); //Force memory allignment
        ELFImgInfo &curImgInfo = elfImgInfoMap[fileId];

        //INFO_LOGS("elfImgInfoMap[%zd].pltAddr=%p", fileId, curImgInfo.pltStartAddr);
        //INFO_LOGS("elfImgInfoMap.size=%zd",elfImgInfoMap.getSize());
        curImgInfo.firstSymIndex = allExtSymbol.getSize();
        //Allocate space for all rela entries in this file
        //DBG_LOGS("First sym index=%ld", curImgInfo.firstSymIndex);

        adjustMemPerm(pltSection.startAddr, pltSection.startAddr + pltSection.size, PROT_READ | PROT_WRITE | PROT_EXEC);
        adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);

        if (pltSecureSection.startAddr) {
//            DBG_LOGS("Adjusting mem permission from:%p to:%p", pltSecureSection.internalArr,
//                     pltSecureSection.internalArr + pltSecureSection.size);
            adjustMemPerm(pltSecureSection.startAddr, pltSecureSection.startAddr + pltSecureSection.size,
                          PROT_READ | PROT_WRITE | PROT_EXEC);
        }
        std::stringstream ss;
        ss << scaler::ExtFuncCallHook::instance->folderName << "/symbolInfo.txt";

        bool symbolInfoFileExistsBefore = false;
        if (access(ss.str().c_str(), F_OK) == 0) {
            symbolInfoFileExistsBefore = true;
        }
        FILE *symInfoFile = fopen(ss.str().c_str(), "a");
        if (!symInfoFile) { fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }

        if (!symbolInfoFileExistsBefore) {
            fprintf(symInfoFile, "%s,%s,%s\n", "funcName", "globalFileId", "symIdInFile");
        }
        assert(pltSection.size / pltSection.entrySize == parser.relaPLTEntrySize + 1);

        uint8_t *baseAddr = (uint8_t *) pmParser.getPmEntry(fileEntry.pmEntryRange[0].first).addrStart;
        uint8_t *endAddr = (uint8_t *) pmParser.getPmEntry(fileEntry.pmEntryRange.back().second).addrEnd;
        ssize_t validRelaPLTEntrySize = 0;
        bool parseRelaPLTSuccess = parseRela(validRelaPLTEntrySize, SymbolType::C_PLT_API, parser, curImgInfo, fileId,
                                             pltSection, pltSecureSection, parser.relaPLTSection,
                                             parser.relaPLTEntrySize,
                                             symInfoFile, baseAddr, baseAddr, endAddr);
        if (!parseRelaPLTSuccess) {
            ERR_LOG("Failed to parse .rela.plt");
            fclose(symInfoFile);
            return false;
        }
        validRelaPltSize += validRelaPLTEntrySize;

        ssize_t validRelaDYNEntrySize = 0;
        bool parseRelaDYNSuccess = parseRela(validRelaDYNEntrySize, SymbolType::C_DYN_API, parser, curImgInfo, fileId,
                                             pltSection, pltSecureSection, parser.relaDYNSection,
                                             parser.relaDYNEntrySize,
                                             symInfoFile, baseAddr, baseAddr, endAddr);
        if (!parseRelaDYNSuccess) {
            ERR_LOG("Failed to parse .rela.dyn");
            fclose(symInfoFile);
            return false;
        }
        validRelaDynSize += validRelaDYNEntrySize;

        fclose(symInfoFile);
        return true;
    }


    const int SAMPLING_GAP = 0b0;


    bool
    ExtFuncCallHook::shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type, ssize_t &initialGap,
                                          void *&addressOverride) {
        addressOverride = nullptr;
        initialGap = 0;


        ssize_t funcNameLen = strlen(funcName);
        if (funcNameLen == 0) {
            return false;
//            fatalError("Function has no name?!");
        }


        if (scaler::strStartsWith(funcName, "__")) {
            return false;
        }

//

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
            } else if (strncmp(funcName, "fork", 4) == 0) {
                addressOverride = (void *) fork_proxy;
                return true;
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
                //INFO_LOG("dlsym overrided");
                addressOverride = (void *) dlsym_proxy;
                return true;
            }
        } else if (funcNameLen == 6) {
            if (strncmp(funcName, "_ZdlPv", 6) == 0) {
                return false;
            } else if (strncmp(funcName, "dlopen", 6) == 0) {
                //INFO_LOG("dlopen address overrided");
                addressOverride = (void *) dlopen_proxy;
                return true;
            } else if (strncmp(funcName, "dlvsym", 6) == 0) {
                //INFO_LOG("dlopen address overrided");
                addressOverride = (void *) dlvsym_proxy;
                return true;
            }
        } else if (funcNameLen == 7) {
            if (strncmp(funcName, "_dl_sym", 7) == 0) {
                return false;
            } else if (strncmp(funcName, "longjmp", 7) == 0) {
                return false;
            } else if (strncmp(funcName, "_setjmp", 7) == 0) {
                return false;
            } else if (strncmp(funcName, "dlmopen", 7) == 0) {
                addressOverride = (void *) dlmopen_proxy;
                return true;
            }else if (strncmp(funcName, "dlclose", 7) == 0) {
                //INFO_LOG("dlclose address overrided");
                addressOverride = (void *) dlclose_proxy;
                return true;
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
            //else if (strncmp(funcName, "pthread_join", 12) == 0) {
            //    addressOverride = (void *) pthread_join_proxy;
            //    return true;
            //}
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
                INFO_LOG("pthread_create hooked");
                //pthreadCreateSymId = curSymId;
                //todo: Also calculate the time of pthread_create
                //todo: Temporary measure. Make sure pthread_create address is correctly resolved
                //This is important to make sure pthread_create is recorded
                addressOverride = (void *) pthread_create_proxy;
                return true;
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
            }
#ifdef CUDA_ENABLED
                else if (strncmp(funcName, "cuGetProcAddress", 16) == 0) {
                    INFO_LOG("cuGetProcAddress intercepted");
                    addressOverride = (void *) cuGetProcAddress_proxy;
                    return true;
                }
#endif
            else if (strncmp(funcName, "__pthread_unwind", 16) == 0) {
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
//            else if (strncmp(funcName, "pthread_cond_wait", 17) == 0) {
//                addressOverride = (void *) pthread_cond_wait_proxy;
//                return true;
//            }
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
#ifdef CUDA_ENABLED
            else if (strncmp(funcName, "cuGetProcAddress_v2", 19) == 0) {
                INFO_LOG("cuGetProcAddress_v2 intercepted");
                addressOverride = (void *) cuGetProcAddress_proxy;
                INFO_LOGS("cuGetProcAddress_proxy=%p", cuGetProcAddress_proxy);
                return true;
            }
#endif
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
//            else if (strncmp(funcName, "pthread_cond_timedwait", 22) == 0) {
//                addressOverride = (void *) pthread_cond_timedwait_proxy;
//                return true;
//            }
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

        bool bindCorrect = (bind == STB_GLOBAL);// || bind == STB_WEAK);
        bool typeCorrect = (type == STT_FUNC);// || type == STT_NOTYPE);

        if (!bindCorrect || !typeCorrect) {
            return false;
        }
        return true;
    }


    bool ExtFuncCallHook::parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltInfo, ELFSecInfo &pltSecInfo,
                                        ELFSecInfo &gotInfo, FileEntry &fileEntry) {
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

        //Even though a file may consists of multiple PMEntries, we can still only use the first and the last pmEntry to check for address validity.
        uint8_t *baseAddr = (uint8_t *) pmParser.getPmEntry(fileEntry.pmEntryRange[0].first).addrStart;
        uint8_t *endAddr = (uint8_t *) pmParser.getPmEntry(fileEntry.pmEntryRange.back().second).addrEnd;

        pltInfo.startAddr = autoAddBaseAddr((uint8_t *) pltHdr.sh_addr, baseAddr, baseAddr, endAddr);
        //INFO_LOGS("baseAddr=%p pltInfo.startAddr=%p",baseAddr,pltInfo.startAddr);
        gotInfo.startAddr = autoAddBaseAddr((uint8_t *) gotHdr.sh_addr, baseAddr, baseAddr, endAddr);

        if (pltSecInfo.entrySize > 0) {
            //Have .plt.sec table
            pltSecInfo.startAddr = autoAddBaseAddr((uint8_t *) pltSecHdr.sh_addr, baseAddr, baseAddr, endAddr);
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
    const int REC_ARRAY_OFFSET1 = COUNTING_PART_START + 5, DYMAIC_LOADING_OFFSET1_SIZE = 32;
    const int COUNT_OFFSET1 = COUNTING_PART_START + 12, COUNT_OFFSET1_SIZE = 32;
    const int COUNT_OFFSET2 = COUNTING_PART_START + 23, COUNT_OFFSET2_SIZE = 32;
    const int GAP_OFFSET = COUNTING_PART_START + 30, GAP_OFFSET_SIZE = 32;

    const int SKIP_PART_START = COUNTING_PART_START + 45;
    const int GOT_ADDR = SKIP_PART_START + 2, GOT_ADDR_SIZE = 64;
    const int CALL_LD_INST = SKIP_PART_START + 13;
    const int PLT_STUB_ID = SKIP_PART_START + 14, PLT_STUB_ID_SIZE = 32;
    const int PLT_START_ADDR = SKIP_PART_START + 20, PLT_START_ADDR_SIZE = 64;

    const int TIMING_PART_START = SKIP_PART_START + 31;
    const int LOW_BITS_GOTENTRYADDR = TIMING_PART_START + 4;
    const int HIGH_BITS_GOTENTRYADDR = TIMING_PART_START + 12;
    const int SYM_ID = TIMING_PART_START + 21, FUNC_ID_SIZE = 32;
    const int ASM_HOOK_HANDLER_ADDR = TIMING_PART_START + 27, ASM_HOOK_HANDLER_ADDR_SIZE = 64;

    /*
     * If failed, return -1;
    */
    ssize_t ExtFuncCallHook::parsePltStubId(uint8_t *dest) {
        int pushOffset = -1;
        if (*dest == 0xFF) {
            pushOffset = 7;
        } else if (*dest == 0xF3) {
            pushOffset = 5;
        } else {
            fatalError("Plt entry format illegal. Cannot find instruction \"push id\". Is it possible that you inserted a breakpoint in PLT (opcode==0xCC). If the plt entry has been installed by Scaler, then this indicates that the unloading/reloading support in Scaler is borken.");
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

    static ssize_t counter = 0;

    bool ExtFuncCallHook::fillAddrAndSymId2IdSaver(uint8_t **realAddrPtr, uint8_t *firstPltEntry, uint32_t symId,
                                                   uint32_t pltStubId, uint32_t recArrayOffset, uint32_t countOffset,
                                                   uint32_t gapOffset, uint8_t *idSaverEntry) {
        counter += 1;
        //INFO_LOGS("count=%zd",counter);
        assert(sizeof(uint8_t **) == 8);

        memcpy(idSaverEntry + COUNT_TLS_ARR_ADDR, tlsOffset, sizeof(void *));

        //Fill TLS offset (Address filled directly inside)
        memcpy(idSaverEntry + REC_ARRAY_OFFSET1, &recArrayOffset, sizeof(uint32_t));
        memcpy(idSaverEntry + COUNT_OFFSET1, &countOffset, sizeof(uint32_t));
        memcpy(idSaverEntry + COUNT_OFFSET2, &countOffset, sizeof(uint32_t));

        memcpy(idSaverEntry + GAP_OFFSET, &gapOffset, sizeof(uint32_t));

        //Fill got address
        memcpy(idSaverEntry + GOT_ADDR, &realAddrPtr, sizeof(uint8_t **));
        //Fill function id
        memcpy(idSaverEntry + PLT_STUB_ID, &pltStubId, sizeof(uint32_t));
        //Fill first plt address
        memcpy(idSaverEntry + PLT_START_ADDR, &firstPltEntry, sizeof(uint8_t *));

        //INFO_LOG("Here");

        uint32_t realAddrPtrHi = ((uint64_t) realAddrPtr) >> 32;
        uint32_t realAddrPtrLo = ((uint64_t) realAddrPtr) & 0xffffffff;
        //INFO_LOGS("GOT_ADDR=%p", gotAddr);
        //INFO_LOGS("GOT_HI=0x%x GOT_LOW", realAddrPtrHi);
        //INFO_LOGS("GOT_LO=0x%x GOT_LOW", realAddrPtrLo);

        memcpy(idSaverEntry + LOW_BITS_GOTENTRYADDR, &realAddrPtrLo, sizeof(uint32_t));
        memcpy(idSaverEntry + HIGH_BITS_GOTENTRYADDR, &realAddrPtrHi, sizeof(uint32_t));

        //INFO_LOG("Fill Symbol Id");

        //Fill symId
        memcpy(idSaverEntry + SYM_ID, &symId, sizeof(uint32_t));

        //INFO_LOG("Fill Loading Id");

        //Fill loadingId


        //INFO_LOG("Fill asmTimingHandler");

        uint8_t *asmHookPtr = (uint8_t *) &asmTimingHandler;
        //Fill asmTimingHandler
        memcpy(idSaverEntry + ASM_HOOK_HANDLER_ADDR, (void *) &asmHookPtr, sizeof(void *));
        //INFO_LOG("Here");

        return true;
    }


    void ExtFuncCallHook::parseRequiredInfo() {
        pthread_mutex_lock(&dynamicLoadingLock);
        //Initialize existing loading id

        ELFParser elfParser;
        if (!pmParser.parsePMMap()) {
            fatalError("Cannot parsePmMap");
        }
        //Find new file from exising PMMaps
        std::vector<FileID> newFileEntryId;
        pmParser.getNewFileEntryIds(newFileEntryId, true);

        if(tlsOffset== nullptr){
            //Parse TLS offset
            parseTLSOffset();
        }

        //Preallocate elfImgInfoMap
        for (ssize_t i = elfImgInfoMap.getSize(); i < pmParser.getFileEntryArraySize(); ++i) {
            ELFImgInfo *elfImgInfo = elfImgInfoMap.pushBack();
            elfImgInfo->valid = false;
        }
        //Get segment info from /proc/self/maps
        for (FileID fileId = 0; fileId < newFileEntryId.size(); ++fileId) {
            FileID globalFileId = newFileEntryId[fileId];
            FileEntry &curFileEntry = pmParser.getFileEntry(globalFileId);
            INFO_LOGS("Install newly discovered file:%s fileId=%zd", curFileEntry.filePath.c_str(), globalFileId);
            ELFImgInfo &curElfImgInfo = elfImgInfoMap[globalFileId];
            if (elfParser.parse(curFileEntry.filePath.c_str())) {
                //Find the entry size of plt and got
                ELFSecInfo pltInfo{};
                ELFSecInfo pltSecInfo{};
                ELFSecInfo gotInfo{};

                //todo: We assume plt and got entry size are the same.
                if (!parseSecInfos(elfParser, pltInfo, pltSecInfo, gotInfo, curFileEntry)) { fatalError(
                            "Failed to parse API invocation related sections.");
                    exit(-1);
                }
                curElfImgInfo.pltStartAddr = pltInfo.startAddr;
                curElfImgInfo.pltSecStartAddr = pltSecInfo.startAddr;
                curElfImgInfo.gotStartAddr = gotInfo.startAddr;

                if (!parseSymbolInfo(elfParser, globalFileId, curFileEntry, pltInfo, pltSecInfo,
                                     gotInfo)) {
                    fatalErrorS("installation for file %s failed.",
                                                             curFileEntry.filePath.c_str());
                    exit(-1);
                }
                curElfImgInfo.valid = true;
            }
        }
        pthread_mutex_unlock(&dynamicLoadingLock);
    }

    bool ExtFuncCallHook::replacePltEntry() {
        //Allocate callIdSaver
        //todo: memory leak

        if (allExtSymbol.getSize() <= installedSymbolSize) {
            ERR_LOGS(
                    "No new symbols discovered but replacePltEntry was invoked anyways. This is probably caused by dlopen of existing library. allExtSymbol.getSize()=%zd, installedSymbolSize=%zd",
                    allExtSymbol.getSize(),
                    installedSymbolSize);
            return false;
        }

        uint8_t *&relaIdSaver = *relaIdSavers.pushBack();
        //INFO_LOGS("Install %zd symbols",allExtSymbol.getSize()-installedSymbolSize);
        relaIdSaver = (uint8_t *) (mmap(NULL, (allExtSymbol.getSize() - installedSymbolSize) * ID_SAVER_BIN_SIZE,
                                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        uint8_t **relaDynRealAddrSaver = nullptr;
        if (validRelaDynSize <= installedRelaDynSize) {
            DBG_LOG("No new DYN symbols discovered. Will not install this part.");
        } else {
            uint8_t **&_relaDynRealAddrSaver = *relaDynRealAddrSavers.pushBack();
            _relaDynRealAddrSaver = static_cast<uint8_t **>(mmap(NULL, (validRelaDynSize - installedRelaDynSize) *
                                                                       sizeof(uint8_t *), PROT_READ | PROT_WRITE,
                                                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            relaDynRealAddrSaver = _relaDynRealAddrSaver;
        }

        /**
         * Prepare callIdSaver
         */
        adjustMemPerm(relaIdSaver, relaIdSaver + (allExtSymbol.getSize() - installedSymbolSize) * ID_SAVER_BIN_SIZE,
                      PROT_READ | PROT_WRITE | PROT_EXEC);
        uint8_t *curRelaIdSaver = relaIdSaver;
        uint8_t **curRelaDynRealAddrSaver = relaDynRealAddrSaver;
        IdSaverBinWrapper idSaverBinWrapper;
        //Fill address and ids in callIdSaver
        //INFO_LOGS("IdSaverBin=%p", idSaverBinWrapper.idSaverBin);
        ssize_t cachedInstalledSymbolSize = installedSymbolSize;
        for (int curSymId = cachedInstalledSymbolSize; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ExtSymInfo &curSymInfo = allExtSymbol[curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[curSymInfo.fileId];

            if (curSymInfo.symbolType == SymbolType::C_PLT_API) {
                curSymInfo.realAddrPtr = curSymInfo.gotEntryAddr;
            } else if (curSymInfo.symbolType == SymbolType::C_DYN_API) {
                curSymInfo.realAddrPtr = curRelaDynRealAddrSaver;
                curRelaDynRealAddrSaver += 1;
                *curSymInfo.realAddrPtr = *curSymInfo.gotEntryAddr;
            }

            if (curSymInfo.addressOverride) {
                //If address is override, then the real address becomes the overrided address.
                *curSymInfo.realAddrPtr = (uint8_t *) (curSymInfo.addressOverride);
            }
            //Fill relaIdSaver with all instructions
            memcpy(curRelaIdSaver, idSaverBinWrapper.idSaverBin, ID_SAVER_BIN_SIZE);

            //INFO_LOGS("Counter for Symbol:%d is located at %p",curSymId,&curContext->recordingArray.internalArr[curSymId].count);

            if (!fillAddrAndSymId2IdSaver(curSymInfo.realAddrPtr,
                                          curImgInfo.pltStartAddr,
                                          curSymId,
                                          curSymInfo.pltStubId,
                                          LDARR_OFFSET_IN_CONTEXT + INTERNALARR_OFFSET_IN_LDARR,
                                          curSymId * sizeof(RecTuple) + COUNT_OFFSET_IN_RECARR,
                                          curSymId * sizeof(RecTuple) + GAP_OFFSET_IN_RECARR,
                                          curRelaIdSaver)) { fatalError(
                        "fillAddrAndSymId2IdSaver failed, this should not happen");
            }
            curRelaIdSaver += ID_SAVER_BIN_SIZE;
        }
        installedSymbolSize = allExtSymbol.getSize();

        /**
         * Replace plt entry or replace .plt (Or directly replace .plt.sec)
         */
        curRelaIdSaver = relaIdSaver;
        for (int curSymId = cachedInstalledSymbolSize; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ExtSymInfo &curSym = allExtSymbol[curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[curSym.fileId];

            if (curSym.symbolType == SymbolType::C_PLT_API) {
                if (curSym.pltSecEntryAddr) {
                    //Replace .plt.sec
                    if (!fillAddr2pltEntry(curRelaIdSaver, curSym.pltSecEntryAddr)) { fatalError(
                                "pltSecAddr installation failed, this should not happen");
                    }
                } else {
                    //Replace .plt
                    if (!fillAddr2pltEntry(curRelaIdSaver, curSym.pltEntryAddr)) { fatalError(
                                "pltEntry installation failed, this should not happen");
                    }
                }

                //Replace .plt
                if (abs(curSym.pltEntryAddr - *curSym.gotEntryAddr) < 16) {
                    *curSym.realAddrPtr = curRelaIdSaver + CALL_LD_INST;
                }
            } else if (curSym.symbolType == SymbolType::C_DYN_API) {
                adjustMemPerm(curSym.gotEntryAddr, curSym.gotEntryAddr + 1,
                              PROT_READ | PROT_WRITE);
                *curSym.gotEntryAddr = curRelaIdSaver;
            } else { fatalErrorS("Impossible case. Symbol type %d should not appear here", curSym.symbolType);
            }

            curRelaIdSaver += ID_SAVER_BIN_SIZE;
        }

        return true;
    }

    void ExtFuncCallHook::createRecordingFolder() const {
        //sprintf(folderName, "scalerdata_%lu", getunixtimestampms());
        if (mkdir(folderName.c_str(), 0755) == -1) { fatalErrorS("Cannot mkdir %s because: %s", folderName.c_str(),
                                                                 strerror(errno));
        }
    }


    /**
     * Intercept and install dlsym
     * @return successful or not
     */
    bool ExtFuncCallHook::installDlSym(const char *__name, void *realFuncAddr, void *&retAddr) {
        if (!realFuncAddr) {
            ERR_LOGS("dlsym(\"%s\") returns null. Scaler cannot install for this API.", __name);
            retAddr = nullptr;
            return false;
        }
        INFO_LOGS("dlsym hooked to look for %s", __name);
        HookContext *hookContextPtr = curContext;
        //realFuncAddr is valid
        pthread_mutex_lock(&dynamicLoadingLock);

        /**
         * Check if the symbol has been hooked before
         * The symbol is considered "hooked" if the address is the same as before and by the same file (fileId is the same and loadingCounter is the same)
        */
        auto findIter = instance->dlsymRealAddrInstallInfoMap.find(realFuncAddr);
        FileID fileId = instance->pmParser.findFileIdByAddr(realFuncAddr);
        FileEntry &fileEntry = instance->pmParser.getFileEntry(fileId);
        if ((findIter != instance->dlsymRealAddrInstallInfoMap.end())) {
            DlsymInstallInfo &info = findIter->second;

            if (info.fileId == fileId && info.loadingCounter == fileEntry.loadingCounter) {
                //Skip hooking this API because it has been hooked by the same file version
                DBG_LOG("Address was hooked before, Scaler will not install");
                retAddr = info.idSaverEntry;
                pthread_mutex_unlock(&instance->dynamicLoadingLock);
                return false;
            }
        }

        /**
         * Check if them symbol's name matches our requirement
        */
        //Perform installation
        ssize_t initialGap = 0b0;
        void *addressOverride = nullptr;
        Elf64_Word bind = STB_GLOBAL;
        Elf64_Word type = STT_FUNC;

        if (!instance->shouldHookThisSymbol(__name, bind, type, initialGap, addressOverride)) {
            //Should not hook
            retAddr = realFuncAddr;
            pthread_mutex_unlock(&instance->dynamicLoadingLock);
            return false;
        }

        auto pmParserIter = pmParser.findPmEntryIdByAddr(realFuncAddr);
        if (!pmParserIter->isE()) {
            retAddr = realFuncAddr;
            pthread_mutex_unlock(&instance->dynamicLoadingLock);
            return false;
        }

        if (addressOverride) {
            realFuncAddr = addressOverride;
        }

        //Should install, allocate a new saverbinwrapper
        IdSaverBinWrapper *idSaverBinWrapper = dlSymIdSavers.malloc();
        new(idSaverBinWrapper) IdSaverBinWrapper(); //Allocate idsaver
        adjustMemPerm(idSaverBinWrapper->idSaverBin, idSaverBinWrapper->idSaverBin + ID_SAVER_BIN_SIZE,
                      PROT_READ | PROT_WRITE | PROT_EXEC);
        idSaverBinWrapper->realFuncAddr = realFuncAddr;

        ssize_t newSymId = hookContextPtr->recordingArray.getSize();
        hookContextPtr->recordingArray.pushBack(); //Insert new entry to record dlsym loaded symbol
        if (!fillAddrAndSymId2IdSaver((uint8_t **) &idSaverBinWrapper->realFuncAddr, NULL, newSymId, NULL,
                                      LDARR_OFFSET_IN_CONTEXT + INTERNALARR_OFFSET_IN_LDARR,
                                      newSymId * sizeof(RecTuple) + COUNT_OFFSET_IN_RECARR,
                                      newSymId * sizeof(RecTuple) + GAP_OFFSET_IN_RECARR,
                                      idSaverBinWrapper->idSaverBin)) { fatalError(
                    "fillAddrAndSymId2IdSaver failed, this should not happen");
        }
        //Save idSaver address to dlsymRealAddrGOTEntryMap

        const auto emplaceRlt = dlsymRealAddrInstallInfoMap.emplace(std::make_pair(realFuncAddr, DlsymInstallInfo()));
        emplaceRlt.first->second.idSaverEntry = idSaverBinWrapper->idSaverBin;
        emplaceRlt.first->second.fileId = fileId;
        emplaceRlt.first->second.loadingCounter = fileEntry.loadingCounter;
        // INFO_LOGS("Insert dlsym loaded symbol fileId=%zd loadingCounter=%zd symbolId=%zd",fileId,  emplaceRlt.first->second.loadingCounter,newSymId);

        retAddr = idSaverBinWrapper->idSaverBin;

        ExtSymInfo *symInfo = instance->allExtSymbol.pushBack();
        installedSymbolSize += 1;
        //The fileId of dlsym must be resolved right now because library may be unloaded by the user
        symInfo->realFileId = pmParser.findFileIdByAddr(realFuncAddr);
        symInfo->dlsymRealAddr = static_cast<uint8_t *>(realFuncAddr);
        symInfo->symbolType = SymbolType::C_DL_API;

        //Write symbol information to file
        std::stringstream ss;
        ss << scaler::ExtFuncCallHook::instance->folderName << "/symbolInfo.txt";
        bool symbolInfoFileExistsBefore = false;
        if (access(ss.str().c_str(), F_OK) == 0) {
            symbolInfoFileExistsBefore = true;
        }

        FILE *symInfoFile = fopen(ss.str().c_str(), "a");
        if (!symInfoFile) { fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }

        if (!symbolInfoFileExistsBefore) {
            fprintf(symInfoFile, "%s,%s,%s\n", "funcName", "globalFileId", "symIdInFile");
        }

        fprintf(symInfoFile, "%s,%ld,%d\n", __name, symInfo->fileId, -1);
        pthread_mutex_unlock(&dynamicLoadingLock);
        return true;
    }

    bool ExtFuncCallHook::parseRealFileId() {
        //Before DlClose time, we need to parse the real address of parsed symbols
        ssize_t allExtSymbolSize = instance->allExtSymbol.getSize();
        for (ssize_t i = 0; i < allExtSymbolSize; ++i) {
            ExtSymInfo &curExtSymbolInfo = instance->allExtSymbol[i];
            //curExtSymbolInfo.fileId

            if (curExtSymbolInfo.realFileId == -1 && instance->pmParser.getFileEntry(curExtSymbolInfo.fileId).fileExists) {
                //RealFileId has not been resolved yet
                if (curExtSymbolInfo.symbolType == SymbolType::C_DYN_API || curExtSymbolInfo.symbolType == SymbolType::C_PLT_API) {
                    if (curExtSymbolInfo.realAddrPtr == nullptr) {
                        INFO_LOGS("Fatal error occured in symbolId=%zd symbolType=%d", i, curExtSymbolInfo.symbolType);
                    }
                    assert(curExtSymbolInfo.realAddrPtr != nullptr);
                    curExtSymbolInfo.realFileId = instance->pmParser.findFileIdByAddr(*(curExtSymbolInfo.realAddrPtr));
                } else if (curExtSymbolInfo.symbolType == SymbolType::C_DL_API) {
                    //DL realFileId has been resolved at installation time. No need to resolve again.
                } else if (curExtSymbolInfo.symbolType == SymbolType::PY_API) {
                    fatalError("Scaler does not support Python API");
                }
            }
        }


        return true;
    }

    void ExtFuncCallHook::parseTLSOffset() {
        ssize_t scalerFileId = pmParser.getScalerFileId();
        assert(scalerFileId != -1);
        ELFParser parser;
        FileEntry& curFileEntry=pmParser.getFileEntry(scalerFileId);
        parser.parse(curFileEntry.filePath.c_str());

        uint8_t *baseAddr = (uint8_t *) pmParser.getPmEntry(curFileEntry.pmEntryRange[0].first).addrStart;
        uint8_t *endAddr = (uint8_t *) pmParser.getPmEntry(curFileEntry.pmEntryRange.back().second).addrEnd;

        ELFSecInfo pltInfo{};
        ELFSecInfo pltSecInfo{};
        ELFSecInfo gotInfo{};

        //todo: We assume plt and got entry size are the same.
        if (!parseSecInfos(parser, pltInfo, pltSecInfo, gotInfo, curFileEntry)) { fatalError(
                    "Failed to parse TLS offset related sections.");
            exit(-1);
        }
        for (ssize_t i = 0; i < parser.relaDYNEntrySize; ++i) {
            const char *funcName;
            Elf64_Word type;
            Elf64_Word bind;
            parser.getExtSymbolInfo(i, funcName, bind, type, parser.relaDYNSection);
            ssize_t stringLength = strlen(funcName);

            if(stringLength==23 && strncmp(funcName,"_ZN6scaler10curContextE",stringLength)==0){
                tlsOffset = (uint8_t **) autoAddBaseAddr((uint8_t *) (parser.getRelaOffset(i, parser.relaDYNSection)),
                                                                 baseAddr, baseAddr, endAddr);
                break;
            }
        }

    }
}

#endif
