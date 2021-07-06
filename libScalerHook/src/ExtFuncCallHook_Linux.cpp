#ifdef __linux

#include <util/hook/ExtFuncCallHook_Linux.hh>
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
#include <thread>
#include <util/tool/Logging.h>
#include <util/tool/Config.h>

namespace scaler {

    void ExtFuncCallHook_Linux::locateRequiredSecAndSeg() {
        //Get segment info from /proc/self/maps
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            auto &curFileName = iter->first;
            auto &pmEntries = iter->second;
            size_t curFileiD = pmParser.fileIDMap.at(curFileName);
            auto &curELFImgInfo = elfImgInfoMap[curFileiD];
            curELFImgInfo.filePath = curFileName;

            if (curFileName == "") {
                continue;
            }

            try {
                /**
                 * Open corresponding c file
                 * Some sections may throw exception here. So we will naturally skip invalid path like "[stack]" .etc
                 */
                ELFParser_Linux elfParser(curFileName);

                //For .plt, .plt.sec, we only need to search in segments with executable permission
                std::vector<PMEntry_Linux> codeSegments;
                //For _DYNAMIC, we only need to search in segments with readable but not executable permission
                std::vector<PMEntry_Linux> readableNonCodeSegments;
                for (auto &pmEntry : pmEntries) {
                    if (pmEntry.isE)
                        codeSegments.emplace_back(pmEntry);
                    else if (pmEntry.isR)
                        readableNonCodeSegments.emplace_back(pmEntry);
                }

                /**
                 * Get plt from ELF file.
                 */
                auto pltHdr = elfParser.getSecHdrByName(".plt");

                void *pltAddrInFile = elfParser.getSecContent(pltHdr);
                curELFImgInfo.pltStartAddr = memTool->searchBinInMemory(pltAddrInFile, sizeof(pltHdr.secHdr.sh_entsize),
                                                                        codeSegments);
                assert(curELFImgInfo.pltStartAddr);

                //We already have the starting address, let's calculate the end address
                curELFImgInfo.pltEndAddr = (uint8_t *) curELFImgInfo.pltStartAddr
                                           + pltHdr.secHdr.sh_size;
                assert(curELFImgInfo.pltEndAddr);

                /**
                 * Get plt.sec from ELF file. An exception will be thrown if there's no plt.sec table in that elf file.
                 * In this case, we won't hook plt.sec since it doesn't exist. But the hook still works for PLT.
                 */
                try {
                    auto pltSecHdr = elfParser.getSecHdrByName(".plt.sec");
                    void *pltSecAddrInFile = elfParser.getSecContent(pltSecHdr);
                    curELFImgInfo.pltSecStartAddr = memTool->searchBinInMemory(pltSecAddrInFile, 32,
                                                                               codeSegments);

                    //We already have the starting address, let's calculate the end address
                    curELFImgInfo.pltSecEndAddr = (uint8_t *) curELFImgInfo.pltSecStartAddr
                                                  + pltSecHdr.secHdr.sh_size;

                } catch (const ScalerException &e) {
                    if (e.code != ELFParser_Linux::ErrCode::SYMBOL_NOT_FOUND) {
                        throw e;
                    } else {
                        //Clear addr because it has no .plt.sec
                        curELFImgInfo.pltSecStartAddr = nullptr;
                    }
                }

                /**
                * Get _DYNAMIC segment. This may throw exception if there's no such structure in that elf file.
                * This is a very important structure to find symbol table and relocation table in memory
                * _DYNAMIC should always exist.
                */
                auto dynamicHdr = elfParser.getProgHdrByType(PT_DYNAMIC);
                assert(dynamicHdr.size() == 1); //There should be only one _DYNAMIC
                void *dynamicAddrInFile = elfParser.getSegContent(dynamicHdr[0]);
                curELFImgInfo._DYNAMICAddr = static_cast<Elf64_Dyn *>(dynamicAddrInFile);
                //curELFImgInfo._DYNAMICAddr = static_cast<ElfW(Dyn) *>(searchBinInMemory(dynamicAddrInFile,
                //                                                                        sizeof(ElfW(Dyn)),
                //                                                                        readableNonCodeSegments));
                assert(curELFImgInfo._DYNAMICAddr);

                uint8_t *curBaseAddr = pmParser.fileBaseAddrMap.at(curFileiD);
                curELFImgInfo.baseAddr = curBaseAddr;

                const ElfW(Dyn) *dynsymDyn = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_SYMTAB);

                if (dynsymDyn == nullptr) {
                    std::stringstream ss;
                    ss << "Cannot find symtab in \"" << curELFImgInfo.filePath << "\"";
                    throwScalerException(ss.str().c_str());
                }

                curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, dynsymDyn->d_un.d_ptr);
                curELFImgInfo.dynSymTable = (const ElfW(Sym) *) (curBaseAddr + dynsymDyn->d_un.d_ptr);

                const ElfW(Dyn) *strTabDyn = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRTAB);
                if (strTabDyn == nullptr) {
                    std::stringstream ss;
                    ss << "Cannot find strtab in \"" << curELFImgInfo.filePath << "\"";
                    throwScalerException(ss.str().c_str());
                }
                curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, strTabDyn->d_un.d_ptr);
                curELFImgInfo.dynStrTable = (const char *) (curBaseAddr + strTabDyn->d_un.d_ptr);

                const ElfW(Dyn) *strSizeDyn = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRSZ);
                if (strSizeDyn == nullptr) {
                    std::stringstream ss;
                    ss << "Cannot find strtab size in \"" << curELFImgInfo.filePath << "\"";
                    throwScalerException(ss.str().c_str());
                }
                curELFImgInfo.dynStrSize = strSizeDyn->d_un.d_val;

                ElfW(Dyn) *relaPltDyn = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_JMPREL);
                if (relaPltDyn == nullptr) {
                    std::stringstream ss;
                    ss << "Cannot find .plt.rela in \"" << curELFImgInfo.filePath << "\"";
                    throwScalerException(ss.str().c_str());
                }
                curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, relaPltDyn->d_un.d_ptr);
                curELFImgInfo.relaPlt = (ElfW(Rela) *) (curBaseAddr + relaPltDyn->d_un.d_ptr);

                const ElfW(Dyn) *relaSizeDyn = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_PLTRELSZ);
                if (relaSizeDyn == nullptr) {
                    std::stringstream ss;
                    ss << "Cannot find .plt.rela size in \"" << curELFImgInfo.filePath << "\"";
                    throwScalerException(ss.str().c_str());
                }
                curELFImgInfo.relaPltCnt = relaSizeDyn->d_un.d_val / sizeof(ElfW(Rela));

                std::stringstream ss;

                for (size_t i = 0; i < curELFImgInfo.relaPltCnt; ++i) {
                    ElfW(Rela) *curRelaPlt = curELFImgInfo.relaPlt + i;
                    //assert(ELF64_R_TYPE(curRelaPlt->r_info) == R_X86_64_JUMP_SLOT);
                    //todo: Used ELF64 here
                    size_t idx = ELF64_R_SYM(curRelaPlt->r_info);
                    idx = curELFImgInfo.dynSymTable[idx].st_name;

                    if (idx + 1 > curELFImgInfo.dynStrSize) {
                        throwScalerException("Too big section header string table index");
                    }
                    //printf("%s:%s\n", curFileName.c_str(), std::string(curELFImgInfo.dynStrTable + idx).c_str());
                    try {
                        ExtSymInfo newSymbol;
                        newSymbol.symbolName = std::string(curELFImgInfo.dynStrTable + idx);

                        curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, curRelaPlt->r_offset);
                        newSymbol.gotEntry = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

                        if (newSymbol.gotEntry == nullptr) {
                            throwScalerException("Failed to get got address.");
                        }

                        newSymbol.fileId = curFileiD;
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
                        elfImgInfoMap.erase(curFileiD);
                        std::stringstream ss;
                        ss << "Hook Failed for " << curFileName << "in\"" << curELFImgInfo.filePath << "\" because "
                           << e.info;
                        fprintf(stderr, "%s\n", ss.str().c_str());
                    }

                }

            } catch (const ScalerException &e) {
                //Remove current entry
                elfImgInfoMap.erase(curFileiD);
                std::stringstream ss;
                ss << "Hook Failed for \"" << curFileName << "\" because " << e.info;
                fprintf(stderr, "%s\n", ss.str().c_str());
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


    ExtFuncCallHook_Linux::ExtFuncCallHook_Linux() {

    }

    ExtFuncCallHook_Linux::~ExtFuncCallHook_Linux() {

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
