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

namespace scaler {

    bool ExtFuncCallHook::install() {
        //Parse filenames
        pmParser.parsePMMap();
        ELFParser elfParser;

        //Push a guard entry in this case we don't need to specifically handle the last element
        pmParser.pmEntryArray.pushBack()->fileId = -1;

        ssize_t prevFileId = -1;
        uint8_t *prevFileBaseAddr = pmParser.pmEntryArray[0].addrStart;
        std::string curFileName;

        for (int i = 0; i < pmParser.pmEntryArray.getSize(); ++i) {
            PMEntry &curPmEntry = pmParser.pmEntryArray[i];
            //DBG_LOGS("%hd", curPmEntry.fileId);
        }

        //Get segment info from /proc/self/maps
        for (int i = 0; i < pmParser.pmEntryArray.getSize(); ++i) {
            PMEntry &curPmEntry = pmParser.pmEntryArray[i];

            if (prevFileId != -1 && prevFileId != curPmEntry.fileId) {
                //A new file discovered
                DBG_LOGS("%s", curFileName.c_str());

                //Find the entry size of plt and got
                ELFSecInfo pltInfo;
                ELFSecInfo gotInfo;

                //todo: We assume plt and got entry size is the same.
                if (!parseSecInfos(elfParser, pltInfo, gotInfo, prevFileBaseAddr)) {
                    fatalError("Failed to parse plt related sections.");
                    exit(-1);
                }

                //Install hook on this file
                if (!installHook(curFileName, elfParser, prevFileId, prevFileBaseAddr, pltInfo, gotInfo)) {
                    fatalErrorS("installation for file %s failed.", curFileName.c_str());
                    exit(-1);
                }

                prevFileBaseAddr = pmParser.pmEntryArray[i].addrStart;
                ELFImgInfo *curElfImgInfo = elfImgInfoMap.pushBack();
                curElfImgInfo->pltStartAddr = pltInfo.startAddr;
                curElfImgInfo->gotStartAddr = gotInfo.startAddr;
            }

            //Move on to the next entry
            if (curPmEntry.fileId != -1) {
                prevFileId = curPmEntry.fileId;

                curFileName = pmParser.fileNameArr[curPmEntry.fileId];
                if (!elfParser.parse(curFileName.c_str())) {
                    fatalErrorS("Failed to parse elf file: %s", curFileName.c_str());
                    exit(-1);
                }
            } else {
                //This is the guard entry, break
                break;
            }
        }
        //Remove guard
        pmParser.pmEntryArray.popBack();
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

    ExtFuncCallHook *ExtFuncCallHook::getInst() {
        if (!instance) {
            instance = new ExtFuncCallHook();
            if (!instance) {
                fatalError("Cannot allocate memory for ExtFuncCallHookAsm");
                return nullptr;
            }
        }
        return instance;
    }


    ExtFuncCallHook::~ExtFuncCallHook() {
        //todo: release oriPltCode oriPltSecCode
        uninstall();
    }

    ExtFuncCallHook::ExtFuncCallHook(PmParser &parser) : pmParser(parser),
                                                         elfImgInfoMap() {

    }

    ExtFuncCallHook::ExtFuncCallHook() {

    }

    bool ExtFuncCallHook::makeGOTWritable(ELFSecInfo &gotSec, bool writable) {
        if (writable) {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);
        } else {
            return adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ);
        }
    }

    bool ExtFuncCallHook::installHook(std::string &fullPath, ELFParser &parser, ssize_t fileId,
                                      uint8_t *baseAddr, ELFSecInfo &pltSec, ELFSecInfo &gotSec) {

        adjustMemPerm(gotSec.startAddr, gotSec.startAddr + gotSec.size, PROT_READ | PROT_WRITE);
        printf("%s\n", fullPath.c_str());
        //makeGOTWritable(gotSec, true);

        for (ssize_t i = 0; i < parser.relaEntrySize; ++i) {
            const char *funcName = parser.getExtSymbolName(i);
            if (!shouldHookThisSymbol(funcName)) {
                continue;
            }
            uint8_t **gotAddr = reinterpret_cast<uint8_t **>(parser.getRelaOffset(i) + baseAddr);
            uint8_t *curGotDest = *gotAddr;
            bool addressResolved = (curGotDest - pltSec.startAddr) > pltSec.size;
            printf("%s %p %p %s\n", funcName, gotAddr, *gotAddr, addressResolved ? "Resolved" : "Unresolved");

            //GOT replacement
            switch (fileId) {
                case 0:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper0);
                    break;
                case 1:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper1);
                    break;
                case 2:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper2);
                    break;
                case 3:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper3);
                    break;
                case 4:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper4);
                    break;
                case 5:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper5);
                    break;
                case 6:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper6);
                    break;
                case 7:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper7);
                    break;
                case 8:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper8);
                    break;
                case 9:
                    *gotAddr = reinterpret_cast<uint8_t *>(redzoneJumper9);
                    break;
                default:
                fatalErrorS("File id %zd >10. Please increase redzone jumper size", fileId);
            }


        }
        return true;
    }


    bool ExtFuncCallHook::shouldHookThisSymbol(const char *funcName) {
        ssize_t funcNameLen = strlen(funcName);

        if (funcNameLen == 5 && strncmp(funcName, "funcA", 5) == 0) {
            return true;
        }
        return false;

        if (funcNameLen == 3) {
            if (strncmp(funcName, "oom", 3) == 0) {
                return false;
            } else if (strncmp(funcName, "err", 3) == 0) {
                return false;
            }
        } else if (funcNameLen == 4) {
            if (strncmp(funcName, "jump", 4) == 0) {
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
            if (strncmp(funcName, "_exit", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "abort", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "_Exit", 5) == 0) {
                return false;
            } else if (strncmp(funcName, "verrx", 5) == 0) {
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


    bool ExtFuncCallHook::parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltSecInfo, ELFSecInfo &gotSecInfo,
                                        uint8_t *baseAddr) {
        Elf64_Shdr pltSecHdr;
        if (!elfParser.getSecHeader(SHT_PROGBITS, ".plt", pltSecHdr)) {
            ERR_LOG("Cannot read .plt header");
            return false;
        }
        pltSecInfo.size = pltSecHdr.sh_size;
        pltSecInfo.entrySize = pltSecHdr.sh_entsize;

        Elf64_Shdr gotSecHdr;
        if (!elfParser.getSecHeader(SHT_PROGBITS, ".got", gotSecHdr)) {
            ERR_LOG("Cannot read .got header");
            return false;
        }
        gotSecInfo.size = gotSecHdr.sh_size;
        gotSecInfo.entrySize = gotSecHdr.sh_entsize;


        pltSecInfo.startAddr = static_cast<uint8_t *>(elfParser.parseSecLoc(pltSecHdr, baseAddr));
        gotSecInfo.startAddr = static_cast<uint8_t *>(elfParser.parseSecLoc(gotSecHdr, baseAddr));
        return pltSecInfo.startAddr != nullptr && gotSecInfo.startAddr != nullptr;
    }


}

#endif