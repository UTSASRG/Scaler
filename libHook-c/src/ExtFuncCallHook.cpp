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
#include <util/tool/AddrFileIdMapping.h>

namespace scaler {


    bool ExtFuncCallHook::install() {

        //Parse filenames
        pmParser.parsePMMap();
        //Get pltBaseAddr

        parseRequiredInfo();

        replacePltEntry();

        if (!initTLS()) {
            ERR_LOG("Failed to initialize TLS");
            //This is the main thread
        }

        DBG_LOG("Finished installation");

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

    bool ExtFuncCallHook::parseSymbolInfo(ELFParser &parser, ssize_t fileId, uint8_t *baseAddr, ELFSecInfo &pltSection,
                                          ELFSecInfo &pltSecureSection, ELFSecInfo &gotSec) {

        ELFImgInfo &curImgInfo = elfImgInfoMap[fileId];
        curImgInfo.firstSymIndex = allExtSymbol.getSize();
        //Allocate space for all rela entries in this file
        //DBG_LOGS("First sym index=%ld", curImgInfo.firstSymIndex);

        adjustMemPerm(pltSection.startAddr, pltSection.startAddr + pltSection.size, PROT_READ | PROT_WRITE | PROT_EXEC);

        if (pltSecureSection.startAddr) {
//            DBG_LOGS("Adjusting mem permission from:%p to:%p", pltSecureSection.startAddr,
//                     pltSecureSection.startAddr + pltSecureSection.size);
            adjustMemPerm(pltSecureSection.startAddr, pltSecureSection.startAddr + pltSecureSection.size,
                          PROT_READ | PROT_WRITE | PROT_EXEC);
        }


        assert(pltSection.size / pltSection.entrySize == parser.relaEntrySize + 1);
        for (ssize_t i = 0; i < parser.relaEntrySize; ++i) {
            const char *funcName;
            Elf64_Word type;
            Elf64_Word bind;
            parser.getExtSymbolInfo(i, funcName, bind, type);
            if (!shouldHookThisSymbol(funcName, bind, type)) {
                continue;
            }
            //Get function id from plt entry

            uint8_t **gotAddr = reinterpret_cast<uint8_t **>(parser.getRelaOffset(i) + baseAddr);
            uint8_t *curGotDest = *gotAddr;
            uint8_t *pltSecEntry = nullptr;

            if (curImgInfo.pltSecStartAddr) {
                pltSecEntry = curImgInfo.pltSecStartAddr + pltSecureSection.entrySize * i;
            }
            uint8_t *pltEntry = curImgInfo.pltStartAddr + pltSection.entrySize * (i + 1);


            uint32_t pltStubId = parsePltStubId(pltEntry); //Note that the first entry is not valid

            //Make sure space is enough, if space is enough, array won't allocate
            ExtSymInfo *newSym = allExtSymbol.pushBack();


            newSym->addrResolved = abs(curGotDest - pltSection.startAddr) > pltSection.size;
            newSym->fileId = fileId;
            newSym->symIdInFile = i;
            newSym->gotEntryAddr = gotAddr;
            newSym->pltEntryAddr = pltEntry;
            newSym->pltSecEntryAddr = pltSecEntry;
            newSym->pltStubId = pltStubId;

            DBG_LOGS(
                    "id:%ld funcName:%s gotAddr:%p *gotAddr:%p addressResolved:%s fileId:%zd symIdInFile:%zd pltEntryAddr:%p pltSecEntryAddr:%p",
                    allExtSymbol.getSize() - 1, funcName, gotAddr, *gotAddr,
                    newSym->addrResolved ? "Resolved" : "Unresolved", fileId,
                    newSym->symIdInFile, newSym->pltEntryAddr, newSym->pltSecEntryAddr);
        }

        return true;
    }


    bool ExtFuncCallHook::shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type) {
        if (bind != STB_GLOBAL || type != STT_FUNC) {
            return false;
        }

        ssize_t funcNameLen = strlen(funcName);
        if (funcNameLen == 0) {
            return false;
//            fatalError("Function has no name?!");
        }



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
            } else if (strncmp(funcName, "dlsym", 5) == 0) {
                return false;
            }
        } else if (funcNameLen == 6) {
            if (strncmp(funcName, "_ZdlPv", 6) == 0) {
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


    bool ExtFuncCallHook::parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltInfo, ELFSecInfo &pltSecInfo,
                                        ELFSecInfo &gotInfo,
                                        uint8_t *baseAddr) {
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


        pltInfo.startAddr = static_cast<uint8_t *>(elfParser.parseSecLoc(pltHdr, baseAddr));
        gotInfo.startAddr = static_cast<uint8_t *>(elfParser.parseSecLoc(gotHdr, baseAddr));

        if (pltSecInfo.entrySize > 0) {
            //Have .plt.sec table
            pltSecInfo.startAddr = static_cast<uint8_t *>(elfParser.parseSecLoc(pltSecHdr, baseAddr));
        } else {
            pltSecInfo.startAddr = nullptr;
        }

        return pltInfo.startAddr != nullptr && gotInfo.startAddr != nullptr;
    }

    //16bytes aligned. 0x90 are for alignment purpose
    uint8_t pltEntryBin[] = {0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x41, 0xff, 0xE3, 0x90, 0x90, 0x90};
    //32bytes aligned. 0x90 are for alignment purpose
    uint8_t idSaverBin[] = {0x68, 0x00, 0x00, 0x00, 0x00, 0x49, 0xBB, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41,
                            0xFF, 0xE3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

    uint8_t pltJumperBin[] = {0x68, 0x00, 0x00, 0x00, 0x00, 0x49, 0xBB, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41,
                              0xFF, 0xE3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
                              0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

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

    bool ExtFuncCallHook::fillAddrAndSymId2IdSaver(uint8_t *prehookAddr, uint32_t funcId, uint8_t *idSaverEntry) {

        //Copy code
        memcpy(idSaverEntry, idSaverBin, sizeof(idSaverBin));
        //Copy funcId
        assert(sizeof(funcId) == 4);
        memcpy(idSaverEntry + 1, &funcId, sizeof(funcId));
        //Copy instructionId
        assert(sizeof(uint8_t **) == 8);
        memcpy(idSaverEntry + 7, &prehookAddr, sizeof(uint8_t **));
        return true;
    }

    void ExtFuncCallHook::parseRequiredInfo() {
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

                //Find the entry size of plt and got
                ELFSecInfo pltInfo;
                ELFSecInfo pltSecInfo;
                ELFSecInfo gotInfo;

                //todo: We assume plt and got entry size is the same.
                if (!parseSecInfos(elfParser, pltInfo, pltSecInfo, gotInfo, prevFileBaseAddr)) {
                    fatalError("Failed to parse plt related sections.");
                    exit(-1);
                }
                ELFImgInfo *curElfImgInfo = elfImgInfoMap.pushBack();
                curElfImgInfo->pltStartAddr = pltInfo.startAddr;
                curElfImgInfo->pltSecStartAddr = pltSecInfo.startAddr;
                curElfImgInfo->gotStartAddr = gotInfo.startAddr;

                DBG_LOGS("%zd:%s %p pltSecStartAddr=%p", prevFileId, pmParser.fileNameArr[prevFileId].c_str(),
                         prevFileBaseAddr, pltSecInfo.startAddr);

                //Install hook on this file
                if (!parseSymbolInfo(elfParser, prevFileId, prevFileBaseAddr, pltInfo, pltSecInfo, gotInfo)) {
                    fatalErrorS("installation for file %s failed.", curFileName.c_str());
                    exit(-1);
                }
                prevFileBaseAddr = curPmEntry.addrStart;
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
    }

    bool ExtFuncCallHook::replacePltEntry() {
        //Allocate callIdSaver
        //todo: memory leak
        callIdSavers = static_cast<uint8_t *>(mmap(NULL, allExtSymbol.getSize() * sizeof(idSaverBin),
                                                   PROT_READ | PROT_WRITE,
                                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        //Allocate ldCaller (ldCaller has the smae structure as idSaver)
        ldCallers = static_cast<uint8_t *>(mmap(NULL, allExtSymbol.getSize() * sizeof(idSaverBin),
                                                PROT_READ | PROT_WRITE,
                                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        /**
         * Prepare callIdSaver
         */
        adjustMemPerm(callIdSavers, callIdSavers + allExtSymbol.getSize() * sizeof(idSaverBin),
                      PROT_READ | PROT_WRITE | PROT_EXEC);
        uint8_t *curCallIdSaver = callIdSavers;
        //Fill address and ids in callIdSaver
        for (int curSymId = 0; curSymId < allExtSymbol.getSize(); ++curSymId) {

            if (!fillAddrAndSymId2IdSaver((uint8_t *) &asmHookHandler, curSymId,
                                          curCallIdSaver)) {
                fatalError("fillAddrAndSymId2IdSaver failed, this should not happen");
            }
            curCallIdSaver += sizeof(idSaverBin);
        }


        /**
         * Prepare ldcaller
         */
        adjustMemPerm(ldCallers, ldCallers + allExtSymbol.getSize() * sizeof(idSaverBin),
                      PROT_READ | PROT_WRITE | PROT_EXEC);

        uint8_t *curLdCaller = ldCallers;
        //Fill address and ids in callIdSaver
        for (int curSymId = 0; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ELFImgInfo &curImgInfo = elfImgInfoMap[allExtSymbol[curSymId].fileId];
            if (!fillAddrAndSymId2IdSaver((uint8_t *) curImgInfo.pltStartAddr, allExtSymbol[curSymId].pltStubId,
                                          curLdCaller)) {
                fatalError("fillAddrAndSymId2IdSaver failed, this should not happen");
            }
            curLdCaller += sizeof(idSaverBin);
        }


        /**
         * replace plt entry or replace .plt (Or directly replace .plt.sec)
         */
        curCallIdSaver = callIdSavers;
        for (int curSymId = 0; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ExtSymInfo &curSym = allExtSymbol[curSymId];
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

            curCallIdSaver += sizeof(idSaverBin);

        }

        return true;
    }


}

#endif
