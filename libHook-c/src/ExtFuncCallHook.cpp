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
//        return true;
        //Parse filenames
        pmParser.parsePMMap();
        //Get pltBaseAddr

        Array<PltRange> pltRangeArr(pmParser.pmEntryArray.getSize());

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
                DBG_LOGS("%zd:%s", elfImgInfoMap.getSize(), curFileName.c_str());

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

                PltRange *pltRange = pltRangeArr.pushBack();
                if (pltSecInfo.startAddr == nullptr) {
                    pltRange->addrStart = reinterpret_cast<uint64_t>(pltInfo.startAddr);
                    pltRange->addrEnd = reinterpret_cast<uint64_t>(pltInfo.startAddr + pltInfo.size);
                } else {
                    pltRange->addrStart = (uint64_t) min<uint8_t *>(pltInfo.startAddr, pltSecInfo.startAddr);
                    pltRange->addrEnd = (uint64_t) max<uint8_t *>(pltInfo.startAddr + pltInfo.size,
                                                                  pltSecInfo.startAddr + pltSecInfo.size);
                }


//                //Install hook on this file
//                if (!installHook(elfParser, prevFileId, prevFileBaseAddr, pltInfo, gotInfo)) {
//                    fatalErrorS("installation for file %s failed.", curFileName.c_str());
//                    exit(-1);
//                }
                prevFileBaseAddr = pmParser.pmEntryArray[i].addrStart;
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

        calcBestBucketSize(pltRangeArr, bucketSize, reinterpret_cast<uint64_t &>(hookBaseAddr));


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

    bool ExtFuncCallHook::installHook(ELFParser &parser, ssize_t fileId,
                                      uint8_t *baseAddr, ELFSecInfo &pltSec, ELFSecInfo &gotSec) {

        ELFImgInfo &curImgInfo = elfImgInfoMap[fileId];
        curImgInfo.firstSymIndex = allExtSymbol.getSize();
        //Allocate space for all rela entries in this file
        allExtSymbol.allocate(parser.relaEntrySize);
        //for (ssize_t i = 0; i < parser.relaEntrySize; ++i) {
        //  allExtSymbol.pushBack();
        //}

        adjustMemPerm(pltSec.startAddr, pltSec.startAddr + pltSec.size, PROT_READ | PROT_WRITE | PROT_EXEC);


        assert(pltSec.size / pltSec.entrySize == parser.relaEntrySize + 1);
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
            uint8_t *pltEntry = curImgInfo.pltStartAddr + pltSec.entrySize * (i + 1);

            uint32_t pltStubId = parsePltStubId(pltEntry); //Note that the first entry is not valid
            ExtSymInfo &newSym = allExtSymbol[curImgInfo.firstSymIndex + pltStubId];


            bool addressResolved = (curGotDest - pltSec.startAddr) > pltSec.size;


            newSym.fileId = fileId;
            newSym.symIdInFile = i;
            newSym.hookedId = hookedExtSymSize++;

            DBG_LOGS("funcName:%s gotAddr:%p *gotAddr:%p addressResolved:%s fileId:%zd symIdInFile:%zd hookedId:%zd",
                     funcName, gotAddr, *gotAddr, addressResolved ? "Resolved" : "Unresolved", fileId,
                     newSym.symIdInFile, newSym.hookedId);

            if (addressResolved) {
                newSym.resolvedAddr = *gotAddr;
                newSym.pltLdAddr = *gotAddr;
                newSym.gotEntryAddr = gotAddr;
            } else {
                newSym.resolvedAddr = nullptr;
                newSym.pltLdAddr = *gotAddr;
                newSym.gotEntryAddr = gotAddr;
            }

            //install plt

            int instrLengh = 0;
//            uint8_t *pltEntryStartAddr = (uint8_t *) myPltEntry;
//            if (*pltEntryStartAddr == 0xFF) {
//                //Has endbr
//                instrLengh = 7;
//            } else if (*pltEntryStartAddr == 0xF3) {
//                instrLengh = 5;
//            } else {
//                fatalError("Unrecognized instruction inside myPltEntry. Use disassembler to check.");
//            }
        }

        //Replace plt entry
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
            uint8_t *pltEntry = curImgInfo.pltStartAddr + pltSec.entrySize * (i + 1);

            uint32_t pltStubId = parsePltStubId(pltEntry); //Note that the first entry is not valid
            ExtSymInfo &newSym = allExtSymbol[curImgInfo.firstSymIndex + pltStubId];


            bool addressResolved = (curGotDest - pltSec.startAddr) > pltSec.size;


            newSym.fileId = fileId;
            newSym.symIdInFile = i;
            newSym.hookedId = hookedExtSymSize++;

            DBG_LOGS("funcName:%s gotAddr:%p *gotAddr:%p addressResolved:%s fileId:%zd symIdInFile:%zd hookedId:%zd",
                     funcName, gotAddr, *gotAddr, addressResolved ? "Resolved" : "Unresolved", fileId,
                     newSym.symIdInFile, newSym.hookedId);

            if (addressResolved) {
                newSym.resolvedAddr = *gotAddr;
                newSym.pltLdAddr = *gotAddr;
                newSym.gotEntryAddr = gotAddr;
            } else {
                newSym.resolvedAddr = nullptr;
                newSym.pltLdAddr = *gotAddr;
                newSym.gotEntryAddr = gotAddr;
            }

            fillAddr2pltEntry(reinterpret_cast<uint8_t *>(asmHookHandler), pltEntry);


//            int instrLengh = 0;
//            uint8_t *pltEntryStartAddr = (uint8_t *) myPltEntry;
//            if (*pltEntryStartAddr == 0xFF) {
//                //Has endbr
//                instrLengh = 7;
//            } else if (*pltEntryStartAddr == 0xF3) {
//                instrLengh = 5;
//            } else {
//                fatalError("Unrecognized instruction inside myPltEntry. Use disassembler to check.");
//            }
        }
        //Allocate tls storage, set hook type to FULL


        return true;
    }


    bool ExtFuncCallHook::shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type) {
        if (bind != STB_GLOBAL || type != STT_FUNC) {
            return false;
        }

        if (strncmp(funcName, "funcA", 5) == 0) {
            return true;
        } else {
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

    uint8_t pltEntryBin[] = {0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0xff, 0xD3};

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

    bool ExtFuncCallHook::fillAddr2pltEntry(uint8_t *funcAddr, uint8_t *retPltEntryCode) {
        //Copy code
        memcpy(retPltEntryCode, pltEntryBin, sizeof(pltEntryBin));
        //Copy address
        assert(sizeof(uint8_t **) == 8);
        memcpy(retPltEntryCode + 2, &funcAddr, sizeof(uint8_t **));
        return true;
    }
}

#endif
