#ifdef __linux

#include <util/hook/ExtFuncCallHook.hh>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/ElfParser.h>
#include <exceptions/ScalerException.h>
#include <sys/mman.h>
#include <cassert>
#include <elf.h>
#include <set>
#include <utility>
#include <util/tool/Config.h>
#include <util/tool/MemoryTool_Linux.h>
#include <nlohmann/json.hpp>

namespace scaler {
    bool ExtFuncCallHook::findELFSecInMemory(ELFParser_Linux &elfParser, std::string secName, void *&startAddr,
                                             void *endAddr, void *boundStartAddr, void *boundEndAddr) {
        ELFParser_Linux::SecInfo pltHdr{};
        if (!elfParser.getSecHdrByName(std::move(secName), pltHdr)) {
            //ERR_LOGS("Cannot find elf section %s in memory", secName.c_str());
            return false;
        }

        void *pltAddrInFile = elfParser.getSecContent(pltHdr);
        if (!pltAddrInFile) {
            ERR_LOG("Cannot find elf section in memory");
            return false;
        }
        startAddr = memTool.searchBinInMemory(pltAddrInFile, sizeof(pltHdr.secHdr.sh_entsize),
                                              pmParser.executableSegments, boundStartAddr, boundEndAddr);
        if (!startAddr) {
            ERR_LOGS("Can't find section %s", secName.c_str());
            return false;
        }
        //We already have the starting address, let's calculate the end address
        endAddr = (uint8_t *) startAddr + pltHdr.secHdr.sh_size;
        return true;
    }

    bool ExtFuncCallHook::locateRequiredSecAndSeg() {
        //Get segment info from /proc/self/maps
        for (FileID curFileiD = 0; curFileiD < pmParser.idFileMap.size(); ++curFileiD) {
            auto &curFileName = pmParser.idFileMap.at(curFileiD);
            auto &pmEntries = pmParser.procMap.at(curFileName);

            elfImgInfoMap.pushBack(ELFImgInfo());
            auto &curELFImgInfo = elfImgInfoMap[elfImgInfoMap.getSize() - 1];

            if (curFileName == ""||curFileName[0]=='[') {
                //We don't need noname process entry
                continue;
            }


            /**
             * Open corresponding c file
             * Some sections may throw exception here. So we will naturally skip invalid path like "[stack]" .etc
             */
            ELFParser_Linux elfParser(curFileName);
            if (!elfParser.parse()) {
                ERR_LOGS("Failed to parse elf file: %s", curFileName.c_str());
                continue;
            }

            curELFImgInfo.filePath = curFileName;

            curELFImgInfo.baseAddrStart = pmParser.fileBaseAddrMap.at(curFileiD).first;
            curELFImgInfo.baseAddrEnd = pmParser.fileBaseAddrMap.at(curFileiD).second;

            //Get plt from ELF file
            if (!findELFSecInMemory(elfParser, ".plt", curELFImgInfo.pltStartAddr, curELFImgInfo.pltEndAddr,
                                    curELFImgInfo.baseAddrStart, curELFImgInfo.baseAddrEnd)) {
                ERR_LOGS("Failed to find .plt table in %s. Scaler won't hook this file.", curFileName.c_str());
                continue;
            }

            //DBG_LOGS(".plt section for %s starts at %p",curFileName.c_str(),curELFImgInfo.pltStartAddr);
            //Get .plt.sec (may not exist)
            if (!findELFSecInMemory(elfParser, ".plt.sec", curELFImgInfo.pltSecStartAddr,
                                    curELFImgInfo.pltSecEndAddr, curELFImgInfo.baseAddrStart,
                                    curELFImgInfo.baseAddrEnd)) {
                curELFImgInfo.pltSecStartAddr = nullptr;
                DBG_LOG("Cannot find .plt.sec, this is normal because some machine just doesn't have it.");
            } else {
                DBG_LOGS("%s .plt.sec = %p baseaddr=%p-%p", curELFImgInfo.filePath.c_str(),
                         curELFImgInfo.pltSecStartAddr, curELFImgInfo.baseAddrStart, curELFImgInfo.baseAddrEnd);
            }

            curELFImgInfo._DYNAMICAddr = findDynamicSegment(elfParser);

            curELFImgInfo.dynSymTable = findElemPtrInDynamicSeg<ElfW(Sym) *>(elfParser, curELFImgInfo, curFileiD,
                                                                             DT_SYMTAB);
            if (!curELFImgInfo.dynSymTable) {
                ERR_LOGS("Failed to find dynamic symbol table in %s. Scaler won't hook this file.",
                         curFileName.c_str());
                continue;
            }

            curELFImgInfo.dynStrTable = findElemPtrInDynamicSeg<char *>(elfParser, curELFImgInfo, curFileiD,
                                                                        DT_STRTAB);
            if (!curELFImgInfo.dynStrTable) {
                ERR_LOGS("Failed to find dynamic string table in %s. Scaler won't hook this file.",
                         curFileName.c_str());
                continue;
            }

            curELFImgInfo.dynStrSize = findElemValInDynamicSeg<ssize_t>(elfParser, curELFImgInfo, curFileiD,
                                                                        DT_STRSZ);

            if (!curELFImgInfo.dynStrSize) {
                ERR_LOGS("Failed to find dynamic string size in %s. Scaler won't hook this file.",
                         curFileName.c_str());
                continue;
            }

            curELFImgInfo.relaPlt = findElemPtrInDynamicSeg<ElfW(Rela) *>(elfParser, curELFImgInfo, curFileiD,
                                                                          DT_JMPREL);
            if (!curELFImgInfo.relaPlt) {
                ERR_LOGS("Failed to find .rela.plt in %s. Scaler won't hook this file.", curFileName.c_str());
                continue;
            }

            curELFImgInfo.relaPltCnt = findElemValInDynamicSeg<ssize_t>(elfParser, curELFImgInfo, curFileiD,
                                                                        DT_PLTRELSZ) / sizeof(ElfW(Rela));
            if (!curELFImgInfo.relaPltCnt) {
                ERR_LOGS("Failed to find .rela plt count in %s. Scaler won't hook this file.", curFileName.c_str());
                continue;
            }

            if (!parseRelaSymbol(curELFImgInfo, curFileiD)) {
                ERR_LOGS("Failed to parse relocation table in %s. Scaler won't hook this file.", curFileName.c_str());
                curELFImgInfo.elfImgValid = false;
                continue;
            }

        }

        return true;
        /**
            * We need to know the starting and ending address of sections in other parts of the hook library.
            * Currently we devised three potential ways to do so:
            *
            * 1.Direct Address Calculation
            * We may calculate its address directly. We could find corresponding sections in ELF file by reading section
            * headers. There's an offset field indicating the location of that section in elf file. Program header table
            * describes which part of the ELF file should be loaded into which virtual address. By comparing file offset
            * in section header and file offset in program header. We could know the loading order of sections and which
            * segment a section will be loaded into. By reading /proc/{pid}/maps, we could know the actual loading address
            * of a segment. Thne we use the section order in a segment, section size, segment loading address to calculate
            * the actual address.
            *
            * However, the memory size of a section is not exactly the same as its size in elf file (I have proof).
            * Plus, if PIE is enabled, segments may be loaded randomly into memory. Making its loading address harder
            * to find in /proc/{pid}/maps. These factors make it evem more complicated to implement. Even if we implement this,
            * the code would be too machine-dependent
            *
            * https://uaf.io/exploitation/misc/2016/04/02/Finding-Functions.html
            *
            * 2.Relying on linker script
            * It turns out we could modify linker script to ask linker to mark the starting/ending address of sections.
            * And those marks will be assessable external variables in C/C++ code.
            *
            * This works, but has several drawbacks. First, we can't easily get such variables in other linked executables,
            * making it hard to implement a multi-library plt hook. Second, we need to modify linker script, which will make
            * our program linker-dependent.
            *
            * 3.Search memory.
            * First, we could read elf file to get the binary code for sections. Then, we simply search the memory byte by byte
            * to to find the starting address of such memory.
            *
            * This is implemented in this funciton. It's a more general way to handle this. Plus, it's also fast enough
            * based on the fact that the table we care usually located in the front of a segment.
            */
    }


    ExtFuncCallHook::~ExtFuncCallHook() {

    }

    ExtFuncCallHook::ExtFuncCallHook(PmParser_Linux &parser, MemoryTool_Linux &memTool) : pmParser(parser),
                                                                                          memTool(memTool),
                                                                                          elfImgInfoMap() {

    }

    /**
    * Get _DYNAMIC segment. This may throw exception if there's no such structure in that elf file.
    * This is a very important structure to find symbol table and relocation table in memory
    * _DYNAMIC should always exist.
    */
    ElfW(Dyn) *ExtFuncCallHook::findDynamicSegment(ELFParser_Linux &elfParser) {
        std::vector<ELFParser_Linux::SegInfo> retSegInfoVec;
        if (!elfParser.getProgHdrByType(PT_DYNAMIC, retSegInfoVec) || retSegInfoVec.size() != 1) {
            ERR_LOG("Failed to find header to _DYNAMIC");
            return nullptr;
        }

        ElfW(Dyn) *rltAddr = static_cast<ElfW(Dyn) *>( elfParser.getSegContent(retSegInfoVec[0]));
        if (!rltAddr) {
            ERR_LOG("Failed to find PT_DYNAMIC in ELF file");
            return nullptr;
        }

        return rltAddr;
    }

    bool ExtFuncCallHook::parseRelaSymbol(ELFImgInfo &curELFImgInfo, FileID curFileID) {
        std::stringstream ss;
        for (ssize_t i = 0; i < curELFImgInfo.relaPltCnt; ++i) {
            ElfW(Rela) *curRelaPlt = curELFImgInfo.relaPlt + i;
            //assert(ELF64_R_TYPE(curRelaPlt->r_info) == R_X86_64_JUMP_SLOT);

            ssize_t relIdx = ELFW(R_SYM)(curRelaPlt->r_info);
            ssize_t strIdx = curELFImgInfo.dynSymTable[relIdx].st_name;

            ExtSymInfo newSymbol;
            newSymbol.symbolName = std::string(curELFImgInfo.dynStrTable + strIdx);

            newSymbol.type = ELF64_ST_TYPE(curELFImgInfo.dynSymTable[relIdx].st_info);
            newSymbol.bind = ELF64_ST_BIND(curELFImgInfo.dynSymTable[relIdx].st_info);


            //todo: PLT stub is parsed in "ParsePltEntryAddress"
            newSymbol.pltEntry = nullptr;

            newSymbol.pltSecEntry = nullptr;

            //DBG_LOGS("pltEntryCheck: %s:%s entry is %p", curELFImgInfo.filePath.c_str(),
            //         newSymbol.symbolName.c_str(), newSymbol.pltSecEntry);

            uint8_t *curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddrStart, curFileID,
                                                            curRelaPlt->r_offset);
            newSymbol.gotEntry = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

            if (!newSymbol.gotEntry) {
                ERR_LOG("Failed to get .got address. Cannot hoook");
                return false;
            }

            newSymbol.fileId = curFileID;
            //newSymbol.symIdInFile = i; This should be determined by plt scanning process
            newSymbol.symIdInFile = i;

            if (newSymbol.symbolName == "") {
                //There is no function name parsed. We use func+id to replace function name.
                ss.str("");
                ss << "func" << i;
                newSymbol.symbolName = ss.str();
            }

            newSymbol.scalerSymbolId = allExtSymbol.getSize();
            allExtSymbol.pushBack(newSymbol);

            curELFImgInfo.scalerIdMap.push_back(newSymbol.scalerSymbolId);
        }
        return true;
    }

    bool ExtFuncCallHook::saveAllSymbolId() {
        fatalError("This funciton use an old implementation.");
        return false;
        //drprecated saving function, needs update to be compatible with the latest version
        using Json = nlohmann::json;

        Json outFile;

        for (int i = 0; i < pmParser.idFileMap.size(); ++i) {

            outFile[std::to_string(i)]["fileName"] = pmParser.idFileMap[i];

            outFile[std::to_string(i)]["funcNames"] = Json();
        }


        for (SymID hookedSymId:hookedExtSymbol) {
            const auto &curSymbol = allExtSymbol[hookedSymId];
//            outFile[std::to_string(curSymbol.libraryFileID)]["funcNames"][std::to_string(int64_t(
//                    curSymbol.addr))] = curSymbol.symbolName;
        }

        char fileName[255];
        sprintf(fileName, "symbol_%lu.json", pthread_self());

        FILE *fp = NULL;
        fp = fopen(fileName, "w");
        std::stringstream ss;
        ss << outFile;
        fwrite(ss.str().c_str(), 1, ss.str().size(), fp);
        fclose(fp);

    }

    /**
     * This function will also update the libary fileID in ExtFuncCallHookAsm::hookedExtSymbol
     * @param callerFileID
     * @param symbolIDInCaller
     * @param funcAddr
     * @param libraryFileID
     */
    bool ExtFuncCallHook::parseFuncInfo(FileID callerFileID, FileID symbolIDInCaller, void *&funcAddr,
                                        FileID &libraryFileID) {
        fatalError("Function not implemented");
        return false;
    }


    ExtFuncCallHook::ELFImgInfo::ELFImgInfo() {

    }


    bool ExtFuncCallHook::ELFImgInfo::PthreadFuncId::isFuncPthread(FuncID funcID) {
        return funcID == PTHREAD_CREATE ||
               funcID == PTHREAD_JOIN ||
               funcID == PTHREAD_TRYJOIN_NP ||
               funcID == PTHREAD_TIMEDJOIN_NP ||
               funcID == PTHREAD_CLOCKJOIN_NP ||
               funcID == PTHREAD_MUTEX_LOCK ||
               funcID == PTHREAD_MUTEX_TIMEDLOCK ||
               funcID == PTHREAD_MUTEX_CLOCKLOCK ||
               funcID == PTHREAD_MUTEX_UNLOCK ||
               funcID == PTHREAD_RWLOCK_RDLOCK ||
               funcID == PTHREAD_RWLOCK_TRYRDLOCK ||
               funcID == PTHREAD_RWLOCK_TIMEDRDLOCK ||
               funcID == PTHREAD_RWLOCK_CLOCKRDLOCK ||
               funcID == PTHREAD_RWLOCK_WRLOCK ||
               funcID == PTHREAD_RWLOCK_TRYWRLOCK ||
               funcID == PTHREAD_RWLOCK_TIMEDWRLOCK ||
               funcID == PTHREAD_RWLOCK_CLOCKWRLOCK ||
               funcID == PTHREAD_RWLOCK_UNLOCK ||
               funcID == PTHREAD_COND_SIGNAL ||
               funcID == PTHREAD_COND_BROADCAST ||
               funcID == PTHREAD_COND_WAIT ||
               funcID == PTHREAD_COND_TIMEDWAIT ||
               funcID == PTHREAD_COND_CLOCKWAIT ||
               funcID == PTHREAD_SPIN_LOCK ||
               funcID == PTHREAD_SPIN_TRYLOCK ||
               funcID == PTHREAD_SPIN_UNLOCK ||
               funcID == PTHREAD_BARRIER_WAIT;
    }

    bool ExtFuncCallHook::ELFImgInfo::PthreadFuncId::getAllIds(std::vector<SymID> &retSymId) {
        std::vector<SymID> result;
        if (PTHREAD_CREATE == -1) {
            ERR_LOG("Did not find pthread_create during ELF parsing");
        }
        result.push_back(PTHREAD_CREATE);

        if (PTHREAD_JOIN != -1) {
            ERR_LOG("Did not find pthread_join during ELF parsing");
        }
        result.push_back(PTHREAD_JOIN);
        if (PTHREAD_TRYJOIN_NP != -1) {
            ERR_LOG("Did not find pthread_tryjoinup during ELF parsing");
        }
        result.push_back(PTHREAD_TRYJOIN_NP);
        if (PTHREAD_TIMEDJOIN_NP != -1) {
            ERR_LOG("Did not find pthread_timedjoin_np during ELF parsing");
        }
        result.push_back(PTHREAD_TIMEDJOIN_NP);
        if (PTHREAD_CLOCKJOIN_NP != -1) {
            ERR_LOG("Did not find pthread_clockjoin_np during ELF parsing");
        }
        result.push_back(PTHREAD_CLOCKJOIN_NP);
        if (PTHREAD_MUTEX_LOCK != -1) {
            ERR_LOG("Did not find pthread_mutex_lock during ELF parsing");
        }
        result.push_back(PTHREAD_MUTEX_LOCK);
        if (PTHREAD_MUTEX_TIMEDLOCK != -1) {
            ERR_LOG("Did not find pthread_mutex_timedlock during ELF parsing");
        }
        result.push_back(PTHREAD_MUTEX_TIMEDLOCK);
        if (PTHREAD_MUTEX_CLOCKLOCK != -1) {
            ERR_LOG("Did not find pthread_mutex_clocklock during ELF parsing");
        }
        result.push_back(PTHREAD_MUTEX_CLOCKLOCK);
        if (PTHREAD_MUTEX_UNLOCK != -1) {
            ERR_LOG("Did not find pthread_mutex_unlock during ELF parsing");
        }
        result.push_back(PTHREAD_MUTEX_UNLOCK);
        if (PTHREAD_RWLOCK_RDLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_rdlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_RDLOCK);
        if (PTHREAD_RWLOCK_TRYRDLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_tryrdlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_TRYRDLOCK);
        if (PTHREAD_RWLOCK_TIMEDRDLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_timedrdlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_TIMEDRDLOCK);
        if (PTHREAD_RWLOCK_CLOCKRDLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_clockrdlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_CLOCKRDLOCK);
        if (PTHREAD_RWLOCK_WRLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_wrlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_WRLOCK);
        if (PTHREAD_RWLOCK_TRYWRLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_trywrlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_TRYWRLOCK);
        if (PTHREAD_RWLOCK_TIMEDWRLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_timedwrlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_TIMEDWRLOCK);
        if (PTHREAD_RWLOCK_CLOCKWRLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_clockwrlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_CLOCKWRLOCK);
        if (PTHREAD_RWLOCK_UNLOCK != -1) {
            ERR_LOG("Did not find pthread_rwlock_unlock during ELF parsing");
        }
        result.push_back(PTHREAD_RWLOCK_UNLOCK);
        if (PTHREAD_COND_SIGNAL != -1) {
            ERR_LOG("Did not find pthread_cond_signal during ELF parsing");
        }
        result.push_back(PTHREAD_COND_SIGNAL);
        if (PTHREAD_COND_BROADCAST != -1) {
            ERR_LOG("Did not find pthread_cond_broadcast during ELF parsing");
        }
        result.push_back(PTHREAD_COND_BROADCAST);
        if (PTHREAD_COND_WAIT != -1) {
            ERR_LOG("Did not find pthread_cond_wait during ELF parsing");
        }
        result.push_back(PTHREAD_COND_WAIT);
        if (PTHREAD_COND_TIMEDWAIT != -1) {
            ERR_LOG("Did not find pthread_cond_timedwait during ELF parsing");
        }
        result.push_back(PTHREAD_COND_TIMEDWAIT);
        if (PTHREAD_COND_CLOCKWAIT != -1) {
            ERR_LOG("Did not find pthread_cond_clockwait during ELF parsing");
        }
        result.push_back(PTHREAD_COND_CLOCKWAIT);
        if (PTHREAD_SPIN_LOCK != -1) {
            ERR_LOG("Did not find pthread_spin_lock during ELF parsing");
        }
        result.push_back(PTHREAD_SPIN_LOCK);
        if (PTHREAD_SPIN_TRYLOCK != -1) {
            ERR_LOG("Did not find pthread_spin_trylock during ELF parsing");
        }
        result.push_back(PTHREAD_SPIN_TRYLOCK);
        if (PTHREAD_SPIN_UNLOCK != -1) {
            ERR_LOG("Did not find pthread_spin_unlock during ELF parsing");
        }
        result.push_back(PTHREAD_SPIN_UNLOCK);
        if (PTHREAD_BARRIER_WAIT != -1) {
            ERR_LOG("Did not find pthread_barrier_wait during ELF parsing");
        }
        result.push_back(PTHREAD_BARRIER_WAIT);
        return true;
    }

    bool ExtFuncCallHook::ELFImgInfo::SemaphoreFuncId::isFuncSemaphore(FuncID funcID) {
        return funcID == SEM_WAIT ||
               funcID == SEM_TIMEDWAIT ||
               funcID == SEM_CLOCKWAIT ||
               funcID == SEM_TRYWAIT ||
               funcID == SEM_POST;

    }

    std::vector<SymID> ExtFuncCallHook::ELFImgInfo::SemaphoreFuncId::getAllIds() {
        std::vector<SymID> result;
        if (SEM_WAIT != -1)
            result.push_back(SEM_WAIT);
        if (SEM_TIMEDWAIT != -1)
            result.push_back(SEM_TIMEDWAIT);
        if (SEM_CLOCKWAIT != -1)
            result.push_back(SEM_CLOCKWAIT);
        if (SEM_TRYWAIT != -1)
            result.push_back(SEM_TRYWAIT);
        if (SEM_POST != -1)
            result.push_back(SEM_POST);
        return result;
    }
}


#endif
