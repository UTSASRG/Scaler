#ifdef __linux

#include <util/hook/ExtFuncCallHook_Linux.hh>
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

namespace scaler {
    void ExtFuncCallHook_Linux::findELFSecInMemory(ELFParser_Linux &elfParser, std::string secName, void *&startAddr,
                                                   void *endAddr) {
        auto pltHdr = elfParser.getSecHdrByName(std::move(secName));

        void *pltAddrInFile = elfParser.getSecContent(pltHdr);
        startAddr = memTool.searchBinInMemory(pltAddrInFile, sizeof(pltHdr.secHdr.sh_entsize),
                                              pmParser.executableSegments);
        if (startAddr == nullptr) {
            throwScalerExceptionS(ErrCode::ELF_SECTION_NOT_FOUND_IN_MEM, "Can't find section %s", secName.c_str());
        } else {
            //We already have the starting address, let's calculate the end address
            endAddr = (uint8_t *) startAddr + pltHdr.secHdr.sh_size;
        }
    }

    void ExtFuncCallHook_Linux::locateRequiredSecAndSeg() {
        //Get segment info from /proc/self/maps
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            auto &curFileName = iter->first;
            auto &pmEntries = iter->second;
            auto curFileiD = pmParser.fileIDMap.at(curFileName);
            auto &curELFImgInfo = elfImgInfoMap[curFileiD];
            curELFImgInfo.filePath = curFileName;

            if (curFileName == "") {
                //We don't need noname process entry
                continue;
            }

            try {
                /**
                 * Open corresponding c file
                 * Some sections may throw exception here. So we will naturally skip invalid path like "[stack]" .etc
                 */
                ELFParser_Linux elfParser(curFileName);

                //Get plt from ELF file
                findELFSecInMemory(elfParser, ".plt", curELFImgInfo.pltStartAddr, curELFImgInfo.pltEndAddr);

                //Get .plt.sec (may not exist)
                try {
                    findELFSecInMemory(elfParser, ".plt.sec", curELFImgInfo.pltSecStartAddr,
                                       curELFImgInfo.pltSecEndAddr);
                } catch (const ScalerException &e) {
                    curELFImgInfo.pltSecStartAddr = nullptr;
                    DBG_LOG("Cannot find .plt.sec");
                }

                curELFImgInfo._DYNAMICAddr = findDynamicSegment(elfParser);

                curELFImgInfo.baseAddr = pmParser.fileBaseAddrMap.at(curFileiD);

                curELFImgInfo.dynSymTable = findElemPtrInDynamicSeg<ElfW(Sym) *>(elfParser, curELFImgInfo, curFileiD,
                                                                                 DT_SYMTAB);

                curELFImgInfo.dynStrTable = findElemPtrInDynamicSeg<char *>(elfParser, curELFImgInfo, curFileiD,
                                                                            DT_STRTAB);

                curELFImgInfo.dynStrSize = findElemValInDynamicSeg<size_t>(elfParser, curELFImgInfo, curFileiD,
                                                                           DT_STRSZ);

                curELFImgInfo.relaPlt = findElemPtrInDynamicSeg<ElfW(Rela) *>(elfParser, curELFImgInfo, curFileiD,
                                                                              DT_JMPREL);

                curELFImgInfo.relaPltCnt = findElemValInDynamicSeg<size_t>(elfParser, curELFImgInfo, curFileiD,
                                                                           DT_PLTRELSZ) / sizeof(ElfW(Rela));

                parseRelaSymbol(curELFImgInfo, curFileiD);

            } catch (const ScalerException &e) {
                //Remove current entry
                elfImgInfoMap.erase(curFileiD);
                ERR_LOGS("Hook Failed for \"%s\" because %s", curFileName.c_str(), e.info.c_str());
            }
        }


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


    ExtFuncCallHook_Linux::~ExtFuncCallHook_Linux() {

    }

    ExtFuncCallHook_Linux::ExtFuncCallHook_Linux(PmParser_Linux &parser, MemoryTool_Linux &memTool) : pmParser(parser),
                                                                                                      memTool(memTool) {

    }

    /**
    * Get _DYNAMIC segment. This may throw exception if there's no such structure in that elf file.
    * This is a very important structure to find symbol table and relocation table in memory
    * _DYNAMIC should always exist.
    */
    ElfW(Dyn) *ExtFuncCallHook_Linux::findDynamicSegment(ELFParser_Linux &elfParser) {
        auto dynamicHdr = elfParser.getProgHdrByType(PT_DYNAMIC);
        assert(dynamicHdr.size() == 1); //There should be only one _DYNAMIC
        ElfW(Dyn) *rltAddr = static_cast<ElfW(Dyn) *>( elfParser.getSegContent(dynamicHdr[0]));
        if (rltAddr == nullptr) {
            throwScalerException(ErrCode::ELF_SEGMENT_NOT_FOUND, "Cannot find PT_DYNAMIC in ELF file");
        }
        return rltAddr;
    }

    void ExtFuncCallHook_Linux::parseRelaSymbol(ELFImgInfo &curELFImgInfo, size_t curFileID) {
        std::stringstream ss;
        for (size_t i = 0; i < curELFImgInfo.relaPltCnt; ++i) {
            ElfW(Rela) *curRelaPlt = curELFImgInfo.relaPlt + i;
            //assert(ELF64_R_TYPE(curRelaPlt->r_info) == R_X86_64_JUMP_SLOT);

            size_t idx = ELFW(R_SYM)(curRelaPlt->r_info);
            idx = curELFImgInfo.dynSymTable[idx].st_name;

            if (idx + 1 > curELFImgInfo.dynStrSize) {
                throwScalerException(ErrCode::ELF_DYNAMIC_ENTRY_PARSE_ERR, "Too big section header string table index");
            }

            ExtSymInfo newSymbol;
            try {
                newSymbol.symbolName = std::string(curELFImgInfo.dynStrTable + idx);

                uint8_t *curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddr, curFileID,
                                                                curRelaPlt->r_offset);
                newSymbol.gotEntry = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

                if (newSymbol.gotEntry == nullptr) {
                    throwScalerException(ErrCode::ELF_DYNAMIC_ENTRY_PARSE_ERR, "Failed to get got address.");
                }

                newSymbol.fileId = curFileID;
                newSymbol.funcId = i;

                if (newSymbol.symbolName == "") {
                    ss.str("");
                    ss << "func" << i;
                    newSymbol.symbolName = ss.str();
                }
                curELFImgInfo.idFuncMap[i] = newSymbol.symbolName;
                curELFImgInfo.funcIdMap[newSymbol.symbolName] = i;
                curELFImgInfo.allExtSymbol[i] = newSymbol;
            } catch (const ScalerException &e) {
                //Remove current entry
                elfImgInfoMap.erase(curFileID);
                ERR_LOGS("Hook Failed for \"%s\":\"%s\" because %s", curELFImgInfo.filePath.c_str(),
                         newSymbol.symbolName.c_str(), e.info.c_str());
            }
        }
    }


    ExtFuncCallHook_Linux::ELFImgInfo::~ELFImgInfo() {
        if (realAddrResolvedC)
            delete[] realAddrResolvedC;

        if (hookedExtSymbolC)
            delete[] hookedExtSymbolC;
    }


    ExtFuncCallHook_Linux::ELFImgInfo::ELFImgInfo(const ELFImgInfo &rho) {
        operator=(rho);
    }

    ExtFuncCallHook_Linux::ELFImgInfo::ELFImgInfo() {

    }

    void ExtFuncCallHook_Linux::ELFImgInfo::operator=(const ELFImgInfo &rho) {

        filePath = rho.filePath;
        pltStartAddr = rho.pltStartAddr;
        pltEndAddr = rho.pltEndAddr;
        pltSecStartAddr = rho.pltSecStartAddr;
        pltSecEndAddr = rho.pltSecEndAddr;
        _DYNAMICAddr = rho._DYNAMICAddr;
        realAddrResolved = rho.realAddrResolved;

        realAddrResolvedC = new bool[rho.realAddrResolved.size()];
        memcpy(realAddrResolvedC, rho.realAddrResolvedC, rho.realAddrResolvedCSize * sizeof(bool));
        realAddrResolvedCSize = rho.realAddrResolvedCSize;

        hookedExtSymbolC = new ExtSymInfo[rho.relaPltCnt];
        memcpy(hookedExtSymbolC, rho.hookedExtSymbolC, rho.hookedExtSymbolCSize * sizeof(ExtSymInfo));
        hookedExtSymbolCSize = rho.hookedExtSymbolCSize;

        pseudoPlt = rho.pseudoPlt;

        hookedExtSymbol = rho.hookedExtSymbol;
        allExtSymbol = rho.allExtSymbol;
        funcIdMap = rho.funcIdMap;
        idFuncMap = rho.idFuncMap;

        relaPlt = rho.relaPlt;
        relaPltCnt = rho.relaPltCnt;
        dynSymTable = rho.dynSymTable;
        dynStrTable = rho.dynStrTable;
        dynStrSize = rho.dynStrSize;
        baseAddr = rho.baseAddr;
    }


}


#endif
