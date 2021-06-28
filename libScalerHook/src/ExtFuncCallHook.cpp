#ifdef __linux

#include <util/hook/ExtFuncCallHook.hh>
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
#include <set>
#include <immintrin.h>
#include <thread>

namespace scaler {

#define PUSHXMM(ArgumentName) \
"subq $16,%rsp\n\t" \
"movdqu  %xmm"#ArgumentName" ,(%rsp)\n\t"

#define POPXMM(ArgumentName) \
"movdqu  (%rsp),%xmm"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"


#define PUSHYMM(ArgumentName) \
"subq $32,%rsp\n\t" \
"vmovdqu  %ymm"#ArgumentName" ,(%rsp)\n\t"

#define POPYMM(ArgumentName) \
"vmovdqu  (%rsp),%ymm"#ArgumentName"\n\t"\
"addq $32,%rsp\n\t"


    class Context {
    public:
        //todo: Initialize using maximum stack size
        std::vector<size_t> funcId;
        std::vector<size_t> fileId;
        //Variables used to determine whether it's called by hook handler or not
        bool inHookHanlder = false;
        std::vector<void *> callerAddr;
    };

    class ContextProxy {
    public:

        Context *ctx;
        bool *released = nullptr;

        ContextProxy() {
            //Can't directly use new. Because new will call malloc. If new is hooked, then a hook handler will be called before context is intialized.
            void *ptr = malloc(sizeof(Context));
            ctx = new(ptr) Context();
            ptr = malloc(sizeof(bool));
            released = new(ptr) bool();
            *released = false;
        }

        ~ContextProxy() {
            *released = true;
        }

        void realDeconstructor() {
            //todo: How to execute this in the last minute
//            free(ctx);
//            ctx = nullptr;
//            free(released);
//            released = nullptr;
        }

    };

    thread_local ContextProxy curContext;

    //Declare hook handler written in assembly code

    //void __attribute__((naked)) asmHookHandler();

    void asmHookHandlerSec();

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHook_Linux *ExtFuncCallHook_Linux::instance = nullptr;


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
                curELFImgInfo.pltStartAddr = searchBinInMemory(pltAddrInFile, sizeof(pltHdr.secHdr.sh_entsize),
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
                    curELFImgInfo.pltSecStartAddr = searchBinInMemory(pltSecAddrInFile, 32,
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
                curELFImgInfo._DYNAMICAddr = static_cast<ElfW(Dyn) *>(searchBinInMemory(dynamicAddrInFile,
                                                                                        sizeof(ElfW(Dyn)),
                                                                                        readableNonCodeSegments));
                assert(curELFImgInfo._DYNAMICAddr);

                uint8_t *curBaseAddr = pmParser.fileBaseAddrMap.at(curFileiD);
                curELFImgInfo.baseAddr = curBaseAddr;

                const ElfW(Dyn) *dynsymDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_SYMTAB);
                curBaseAddr = autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, dynsymDyn->d_un.d_ptr);
                curELFImgInfo.dynSymTable = (const ElfW(Sym) *) (curBaseAddr + dynsymDyn->d_un.d_ptr);

                const ElfW(Dyn) *strTabDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRTAB);
                curBaseAddr = autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, strTabDyn->d_un.d_ptr);
                curELFImgInfo.dynStrTable = (const char *) (curBaseAddr + strTabDyn->d_un.d_ptr);

                const ElfW(Dyn) *strSizeDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRSZ);
                curELFImgInfo.dynStrSize = strSizeDyn->d_un.d_val;

                ElfW(Dyn) *relaPltDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_JMPREL);
                curBaseAddr = autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, relaPltDyn->d_un.d_ptr);
                curELFImgInfo.relaPlt = (ElfW(Rela) *) (curBaseAddr + relaPltDyn->d_un.d_ptr);

                const ElfW(Dyn) *relaSizeDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_PLTRELSZ);
                curELFImgInfo.relaPltCnt = relaSizeDyn->d_un.d_val / sizeof(ElfW(Rela));

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
                        if (newSymbol.symbolName == "") {
                            throwScalerException("Failed to get symbol name.");
                        }

                        //printf("Find symbol %s:%s in.rela.plt. Base:%p WBase:%p WOBase:%p\n",
                        //pmParser.idFileMap[curFileiD].c_str(),
                        //newSymbol.symbolName.c_str(),
                        //curELFImgInfo.baseAddr,
                        //curELFImgInfo.baseAddr + curRelaPlt->r_offset,
                        //curRelaPlt->r_offset);

                        curBaseAddr = autoAddBaseAddr(curELFImgInfo.baseAddr, curFileiD, curRelaPlt->r_offset);
                        newSymbol.gotEntry = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

                        if (newSymbol.gotEntry == nullptr) {
                            throwScalerException("Failed to get got address.");
                        }

                        newSymbol.fileId = curFileiD;
                        newSymbol.funcId = i;

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


    void *ExtFuncCallHook_Linux::searchBinInMemory(void *segPtrInFile, size_t firstEntrySize,
                                                   const std::vector<PMEntry_Linux> &segments) {
        void *rltAddr = nullptr;

        for (int i = 0; i < segments.size(); ++i) {
            rltAddr = binCodeSearch(segments[i].addrStart, segments[i].length, segPtrInFile, firstEntrySize);
            if (rltAddr)
                break;
        }
        return rltAddr;
    }

    void ExtFuncCallHook_Linux::install(Hook::SYMBOL_FILTER filterCallB) {
        pmParser.printPM();

        curContext.ctx->inHookHanlder = true;

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //todo:log
        for (auto iterFile = elfImgInfoMap.begin(); iterFile != elfImgInfoMap.end(); ++iterFile) {
            fprintf(stderr, "%s .plt:%p .plt.sec:%p\n", iterFile->second.filePath.c_str(),
                    iterFile->second.pltStartAddr,
                    iterFile->second.pltSecStartAddr);
        }


        //Step2: Change every plt table memory into writeable
        MemoryTool_Linux *memTool = MemoryTool_Linux::getInst();

        //Step3: Use callback to determine which ID to hook
        std::vector<ExtSymInfo> symbolToHook;
        std::set<size_t> fileToHook;

        for (auto iterFile = elfImgInfoMap.begin(); iterFile != elfImgInfoMap.end(); ++iterFile) {
            auto &curFileId = iterFile->first;
            auto &curFileName = pmParser.idFileMap.at(curFileId);
            auto &curElfImgInfo = iterFile->second;

            //loop through external symbols, let user decide which symbol to hook through callback function
            for (auto iterSymbol = curElfImgInfo.idFuncMap.begin();
                 iterSymbol != curElfImgInfo.idFuncMap.end(); ++iterSymbol) {
                auto &curSymbolId = iterSymbol->first;
                auto &curSymbolName = iterSymbol->second;
                if (filterCallB(curFileName, curSymbolName)) {
                    //The user wants this symbol
                    symbolToHook.emplace_back(curElfImgInfo.allExtSymbol.at(curSymbolId));
                    fileToHook.emplace(curFileId);
                }
            }
        }

        //Step4: Build pseodo PLT

        //Allocate pseudo address
        for (auto &curFileID:fileToHook) {
            auto &curELFImgInfo = elfImgInfoMap.at(curFileID);

            //Malloc mem area for pseodo plt
            //todo: 18 is related to binary code. I should use a global config file to store it.
            curELFImgInfo.pseudoPlt = (uint8_t *) malloc(curELFImgInfo.allExtSymbol.size() * 18);

            //todo: debug log
            fprintf(stderr, "Pseudo PLT addr for %s is %p\n", curELFImgInfo.filePath.c_str(), curELFImgInfo.pseudoPlt);

            //printf("mmprotect Allocate pseudo address\n");
            try {
                memTool->adjustMemPerm(curELFImgInfo.pseudoPlt,
                                       curELFImgInfo.pseudoPlt + curELFImgInfo.allExtSymbol.size() * 18,
                                       PROT_READ | PROT_WRITE | PROT_EXEC);
            } catch (const ScalerException &e) {
                std::stringstream ss;
                ss << "Hook Failed, cannot change heap pseudoPlt permission. Exception Info:\""
                   << e.info << "\"";
                fprintf(stderr, "%s\n", ss.str().c_str());
                continue;
            }


        }

        //Step5: Write plt handler code to file
        void *pltHookDl = writeAndCompileHookHanlder(symbolToHook);


        //todo: debug here
        writeAndCompilePseudoPlt(symbolToHook);

        //Step6: Replace PLT table, jmp to dll function
        for (auto &curSymbol:symbolToHook) {

            try {
                memTool->adjustMemPerm(curSymbol.gotEntry, curSymbol.gotEntry + 1,
                                       PROT_READ | PROT_WRITE | PROT_EXEC);
            } catch (const ScalerException &e) {
                std::stringstream ss;
                ss << "Hook Failed for \"" << pmParser.idFileMap.at(curSymbol.fileId) << ":"
                   << curSymbol.symbolName
                   << "\" because " << e.info;
                fprintf(stderr, "%s\n", ss.str().c_str());
                continue;
            }

            //Resolve current address
            curSymbol.addr = *curSymbol.gotEntry;

            auto &curELFImgInfo = elfImgInfoMap.at(curSymbol.fileId);

            //Allocate plt table
            auto binCodeArrPseudoPLT = fillDestAddr2PseudoPltCode(curSymbol.funcId, curELFImgInfo.pltStartAddr);
            //Copy array todo:18 is machine specific


            memcpy(curELFImgInfo.pseudoPlt + 18 * curSymbol.funcId, binCodeArrPseudoPLT.data(), 18);


            //Check if address is already resolved
            size_t symbolFileId = pmParser.findExecNameByAddr(curSymbol.addr);
            //Since it's external symbol, it's address must be in another file.
            curELFImgInfo.realAddrResolved.emplace_back(symbolFileId != curSymbol.fileId);

            curELFImgInfo.hookedExtSymbol[curSymbol.funcId] = curSymbol;


        }

        /**
         * Fill C data strucutre so that we could use them in hook handler
         */
        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter->second;
            curELFImgInfo.realAddrResolvedC = new bool[curELFImgInfo.realAddrResolved.size()];
            curELFImgInfo.realAddrResolvedCSize = curELFImgInfo.realAddrResolved.size();
            for (int i = 0; i < curELFImgInfo.realAddrResolved.size(); ++i) {
                curELFImgInfo.realAddrResolvedC[i] = curELFImgInfo.realAddrResolved[i];
            }
        }

        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter->second;
            curELFImgInfo.hookedExtSymbolC = new ExtSymInfo[curELFImgInfo.relaPltCnt];
            curELFImgInfo.hookedExtSymbolCSize = curELFImgInfo.relaPltCnt;
            for (auto iter = curELFImgInfo.hookedExtSymbol.begin();
                 iter != curELFImgInfo.hookedExtSymbol.end(); ++iter) {
                auto &symbolId = iter->first;
                auto &symbolInfo = iter->second;
                curELFImgInfo.hookedExtSymbolC[symbolId] = symbolInfo;
            }
        }

        //Copy elfImgInfoMap to C data structure
        if (elfImgInfoMapC != nullptr) {
            delete[] elfImgInfoMapC;
        }

        elfImgInfoMapC = new ELFImgInfo[pmParser.fileIDMap.size()];
        elfImgInfoMapCSize = pmParser.fileIDMap.size();

        //Copy everything to c compatible structure.

        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            elfImgInfoMapC[iter->first] = iter->second;
        }


        /**
         * Replace PLT (.plt, .plt.sec) entries
         */
        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter->second;

            for (auto &symbolPair: curELFImgInfo.hookedExtSymbol) {
                auto &curSymbol = symbolPair.second;

                //jmp to custom handler function
                char output[256];
                char *funcName = "pltHandler_%zu_%zu";
                sprintf(output, funcName, curSymbol.fileId, curSymbol.funcId);
                void *pltEntryAddr = dlsym(pltHookDl, output);

                if (pltEntryAddr == NULL) {
                    throwScalerExceptionWithCode("Failed to find pltEntry address from dll", -1);
                }

                auto binCodeArr = fillDestAddr2HookCode(pltEntryAddr);

                //printf("[%s] %s hooked (ID:%d)\n", curELFImgInfo.filePath.c_str(), curSymbol.symbolName.c_str(),
                //       curSymbol.funcId);

                //Step6: Replace .plt.sec and .plt
                //todo: 16 is bin code dependent
                void *dataPtr = binCodeArr.data();

                if (curELFImgInfo.pltSecStartAddr != nullptr) {
                    //.plt.sec table exists
                    try {
                        memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.funcId,
                                               (uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * (curSymbol.funcId + 1),
                                               PROT_READ | PROT_WRITE | PROT_EXEC);
                        memcpy((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * curSymbol.funcId, dataPtr, 16);
                    } catch (const ScalerException &e) {
                        std::stringstream ss;
                        ss << ".plt.sec replacement Failed for \"" << pmParser.idFileMap.at(curSymbol.fileId) << ":"
                           << curSymbol.symbolName
                           << "\" because " << e.info;
                        fprintf(stderr, "%s\n", ss.str().c_str());
                        continue;
                    }
                }


                try {
                    memTool->adjustMemPerm((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.funcId + 1),
                                           (uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.funcId + 1 + 1),
                                           PROT_READ | PROT_WRITE | PROT_EXEC);
                    memcpy((uint8_t *) curELFImgInfo.pltStartAddr + 16 * (curSymbol.funcId + 1), dataPtr, 16);
                } catch (const ScalerException &e) {
                    std::stringstream ss;
                    ss << ".plt replacement Failed for \"" << pmParser.idFileMap.at(curSymbol.fileId) << ":"
                       << curSymbol.symbolName
                       << "\" because " << e.info;
                    fprintf(stderr, "%s\n", ss.str().c_str());
                    continue;
                }

                binCodeArr.clear();

            }

        }
        curContext.ctx->inHookHanlder = false;
    }


    ExtFuncCallHook_Linux::ExtFuncCallHook_Linux() {

    }

    ExtFuncCallHook_Linux *ExtFuncCallHook_Linux::getInst() {
        if (!instance)
            instance = new ExtFuncCallHook_Linux();
        return instance;
    }

    ElfW(Dyn) *ExtFuncCallHook_Linux::findDynEntryByTag(ElfW(Dyn) *dyn, ElfW(Sxword) tag) {
        //In symbol table, the last entry is DT_NULL
        while (dyn->d_tag != DT_NULL) {
            if (dyn->d_tag == tag) {
                return dyn;
            }
            dyn++;
        }
        return nullptr;
    }

    void ExtFuncCallHook_Linux::uninstall() {
        throwScalerException("Uninstall is not implemented.");
    }

    std::vector<uint8_t> ExtFuncCallHook_Linux::fillDestAddr2HookCode(void *funcAddr) {
        //The following "magical" numbers are actually two instructions
        //"movq  $0xFFFFFFFF,%r11\n\t"   73, 187, 0, 0, 0, 0, 0, 0, 0, 0,
        //"call *%r11\n\t" 65, 255, 211
        //"jmp *%r11\n\t" 65, 255, 227 //Jump to dynamic symbol

        //After calling r15. The address of next instruction will be on stack. The hander will pop this address and
        //Calculating function ID by comparing it with the starting address of .plt/.plt.sec address

        std::vector<uint8_t> binCodeArr = {73, 187, 0, 0, 0, 0, 0, 0, 0, 0, 65, 255, 227, 144, 144, 144};

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
        binCodeArr[2] = _funcAddr & h1;
        binCodeArr[3] = (_funcAddr & h2) >> 8;
        binCodeArr[4] = (_funcAddr & h3) >> 16;
        binCodeArr[5] = (_funcAddr & h4) >> 24;
        binCodeArr[6] = (_funcAddr & h5) >> 32;
        binCodeArr[7] = (_funcAddr & h6) >> 40;
        binCodeArr[8] = (_funcAddr & h7) >> 48;
        binCodeArr[9] = (_funcAddr & h8) >> 56;

        return binCodeArr;
    }

    std::vector<uint8_t> ExtFuncCallHook_Linux::fillDestAddr2PseudoPltCode(size_t funcId, void *funcAddr) {
        //The following "magical" numbers are actually two instructions
        //push    0FFFFh   104, 00, 00, 00, 00,
        //mov     r11, 0FFFFFFFFh 73, 187, 00, 00, 00, 00, 00, 00, 00, 00,
        //jmp     r11 65, 255, 227

        std::vector<uint8_t> binCodeArr = {104, 00, 00, 00, 00, 73, 187, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 227};

        const uint64_t h1 = 0b00000000000000000000000011111111;
        const uint64_t h2 = h1 << 8;
        const uint64_t h3 = h1 << 16;
        const uint64_t h4 = h1 << 24;
        const uint64_t h5 = h1 << 32;
        const uint64_t h6 = h1 << 40;
        const uint64_t h7 = h1 << 48;
        const uint64_t h8 = h1 << 56;


        binCodeArr[1] = funcId & h1;
        binCodeArr[2] = (funcId & h2) >> 8;
        binCodeArr[3] = (funcId & h3) >> 16;
        binCodeArr[4] = (funcId & h4) >> 24;

        auto _funcAddr = (ElfW(Addr)) funcAddr;

        binCodeArr[7] = _funcAddr & h1;
        binCodeArr[8] = (_funcAddr & h2) >> 8;
        binCodeArr[9] = (_funcAddr & h3) >> 16;
        binCodeArr[10] = (_funcAddr & h4) >> 24;
        binCodeArr[11] = (_funcAddr & h5) >> 32;
        binCodeArr[12] = (_funcAddr & h6) >> 40;
        binCodeArr[13] = (_funcAddr & h7) >> 48;
        binCodeArr[14] = (_funcAddr & h8) >> 56;

        return binCodeArr;
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
    void *ExtFuncCallHook_Linux::writeAndCompileHookHanlder(std::vector<ExtSymInfo> &symbolToHook) {

        FILE *fp = NULL;

        fp = fopen("./testHandler.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");

        for (auto &curSymbol:symbolToHook) {
            auto &curELFImgInfo = elfImgInfoMap.at(curSymbol.fileId);


            fprintf(fp, "//%s\n", curELFImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curELFImgInfo.idFuncMap.at(curSymbol.funcId).c_str());

            //todo: In GCC < 8, naked function wasn't supported!!
            fprintf(fp, "void  __attribute__((naked)) pltHandler_%zu_%zu(){\n", curSymbol.fileId, curSymbol.funcId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            //Store rsp into r11. We'll later use this value to recover rsp to the correct location
            fprintf(fp, "\"movq (%%rsp),%%r11\\n\\t\"\n");
            //Skip 128-bit redzone. //todo: platform specific. Only on System V 64
            fprintf(fp, "\"subq $128,%%rsp\\n\\t\"\n");

            //Now, everything happens after the redzone
            //Store fileID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.fileId);//fileId
            //Store functionID into stack
            fprintf(fp, "\"pushq  $%lu\\n\\t\"\n", curSymbol.funcId);//funcID
            //Store the original stack location into stack
            fprintf(fp, "\"pushq  %%r11\\n\\t\"\n");

            //jmp to assembly hook handler
            fprintf(fp, "\"movq  $%p,%%r11\\n\\t\"\n", asmHookHandlerSec);
            fprintf(fp, "\"jmp *%%r11\\n\\t\"\n");

            fprintf(fp, ");\n");
            fprintf(fp, "}\n", pos);
        }
        fprintf(fp, "}\n");
        fclose(fp);
        //compile it
        int sysRet = system("gcc -shared -fPIC ./testHandler.cpp -o ./testHandler.so");
        if (sysRet < 0) {
            throwScalerExceptionWithCode("gcc compilation handler failed", sysRet)
        }


        std::stringstream ss;
        ss << pmParser.curExecPath << "/testHandler.so";
        void *handle = dlopen(ss.str().c_str(),
                              RTLD_NOW);
        if (handle == NULL) {
            throwScalerExceptionWithCode("dlOpen failed", sysRet)
        }
        return handle;
    }

    /**
     * todo: This function can be replaced by binary code. But writing this is easier for debugging.
     * Since it's easier to modify.
     *
     * oldrsp-0        caller(return) address
     */
    void *ExtFuncCallHook_Linux::writeAndCompilePseudoPlt(std::vector<ExtSymInfo> &symbolToHook) {
        FILE *fp = NULL;

        fp = fopen("./testPseudoPlt.cpp", "w");
        unsigned int pos = 0;
        const char *name;
        int i;
        fprintf(fp, "extern \"C\"{\n");

        for (auto &curSymbol:symbolToHook) {
            auto &curELFImgInfo = elfImgInfoMap.at(curSymbol.fileId);


            fprintf(fp, "//%s\n", curELFImgInfo.filePath.c_str());
            fprintf(fp, "//%s\n", curELFImgInfo.idFuncMap.at(curSymbol.funcId).c_str());

            fprintf(fp, "void  __attribute__((naked)) pseudoPlt_%zu_%zu(){\n", curSymbol.fileId, curSymbol.funcId);
            fprintf(fp, "__asm__ __volatile__ (\n");

            fprintf(fp, "\"movq  $%p,%%r11\\n\\t\"\n", asmHookHandlerSec);
            fprintf(fp, "\"jmp *%%r11\\n\\t\"\n");

            fprintf(fp, ");\n");
            fprintf(fp, "}\n", pos);
        }
        fprintf(fp, "}\n");
        fclose(fp);
        int sysRet = system("gcc -shared -fPIC ./testPseudoPlt.cpp -o ./testPseudoPlt.so");
        if (sysRet < 0) {
            throwScalerExceptionWithCode("gcc compilation handler failed", sysRet)
        }


        std::stringstream ss;
        ss << pmParser.curExecPath << "/testHandler.so";
        void *handle = dlopen(ss.str().c_str(),
                              RTLD_NOW);
        if (handle == NULL) {
            throwScalerExceptionWithCode("dlOpen failed", sysRet)
        }
        return handle;
    }

    pthread_mutex_t lock0 = PTHREAD_MUTEX_INITIALIZER;

    void *ExtFuncCallHook_Linux::cPreHookHanlderLinuxSec(size_t fileId, size_t funcId, void *callerAddr) {
        pthread_mutex_lock(&lock0);
        //Calculate fileID
        auto &_this = ExtFuncCallHook_Linux::instance;

        auto &curElfImgInfo = _this->elfImgInfoMapC[fileId];

        auto &curSymbol = curElfImgInfo.hookedExtSymbolC[funcId];

        void *retOriFuncAddr = curSymbol.addr;

        if (!curElfImgInfo.realAddrResolvedC[funcId]) {
            void *curAddr = curSymbol.addr;
            void *newAddr = *curSymbol.gotEntry;
            if (curAddr == newAddr) {
                //todo: 18 depends on opCode array
                retOriFuncAddr = curElfImgInfo.pseudoPlt + funcId * 18;
            } else {
                curElfImgInfo.realAddrResolvedC[funcId] = true;
                curSymbol.addr = newAddr;
                retOriFuncAddr = newAddr;
            }
        }

        if (curContext.ctx->inHookHanlder) {
            pthread_mutex_unlock(&lock0);
            return retOriFuncAddr;
        }

        //Starting from here, we could call external symbols and it won't cause any problem
        curContext.ctx->inHookHanlder = true;



        //Push callerAddr into stack
        //curContext.ctx->callerAddr.emplace_back(callerAddr);

        //Push calling info to afterhook
        curContext.ctx->fileId.emplace_back(fileId);
        curContext.ctx->funcId.emplace_back(funcId);


//        for (int i = 0; i < curContext.ctx->fileId.size() * 4; ++i) {
//            printf(" ");
//        }

        //std::stringstream ss;
        //ss << 1;
        // uint64_t id = std::stoull(ss.str());

        printf("[Pre Hook] Thread:%lu File:%s, Func: %s RetAddr:%p\n", 0, _this->pmParser.idFileMap.at(fileId).c_str(),
               curElfImgInfo.idFuncMap.at(funcId).c_str(),retOriFuncAddr);

        curContext.ctx->inHookHanlder = false;
        pthread_mutex_unlock(&lock0);
        return retOriFuncAddr;
    }

    void *ExtFuncCallHook_Linux::cAfterHookHanlderLinux() {
        if (curContext.ctx->inHookHanlder) {
            return nullptr;
        }
        curContext.ctx->inHookHanlder = true;

        auto &_this = ExtFuncCallHook_Linux::instance;

//        for (int i = 0; i < curContext.ctx->fileId.size() * 4; ++i) {
//            printf(" ");
//        }

        size_t fileId = curContext.ctx->fileId.at(curContext.ctx->fileId.size() - 1);
        curContext.ctx->fileId.pop_back();

        auto &curELFImgInfo = _this->elfImgInfoMap.at(fileId);

        auto &fileName = curELFImgInfo.filePath;

        size_t funcId = curContext.ctx->funcId.at(curContext.ctx->funcId.size() - 1);
        curContext.ctx->funcId.pop_back();
        auto &funcName = curELFImgInfo.idFuncMap.at(funcId);

//        std::stringstream ss;
//        ss << std::this_thread::get_id();
//        uint64_t id = std::stoull(ss.str());

        printf("[After Hook] Thread:%lu  File:%s, Func: %s\n", 0, fileName.c_str(), funcName.c_str());

        //void *callerAddr = curContext.callerAddr.at(curContext.callerAddr.size() - 1);
        //curContext.callerAddr.pop_back();


        curContext.ctx->inHookHanlder = false;

//        if (*curContext.released && curContext.ctx->funcId.size() == 0)
//            curContext.realDeconstructor();

        return nullptr;
    }


    ExtFuncCallHook_Linux::~ExtFuncCallHook_Linux() {
        if (elfImgInfoMapC) {
            delete[] elfImgInfoMapC;
        }

    }

    /**
     * Determine whether current elf file use relative address or absolute address
     * @param curBaseAddr
     * @param curFileiD
     * @param targetAddr
     * @return
     */
    uint8_t *ExtFuncCallHook_Linux::autoAddBaseAddr(uint8_t *curBaseAddr, size_t curFileiD, ElfW(Addr) targetAddr) {

        size_t idWithBaseAddr = pmParser.findExecNameByAddr(curBaseAddr + targetAddr);
        size_t idWithoutBaseAddr = pmParser.findExecNameByAddr((void *) targetAddr);
        if (idWithBaseAddr == curFileiD) {
            //Relative
            return curBaseAddr;
        } else if (idWithoutBaseAddr == curFileiD) {
            //Absolute
            return nullptr;
        } else {
            printf("Not found, id1=%d, id2=%d, curFileID=%d\n", idWithBaseAddr, idWithoutBaseAddr, curFileiD);
            assert(false);
        }
    }



//I used to put cHookHandler out of scaler namespace
//However, cHookHandler needs to access variables defined in ExtHook_Linux
//If I use friend function, it cannot be called by Assembly code.
//To make code more elegant, I finally put everything in scaler namespace.
//The cost is I have to find compiled function name once
//    IMPL_ASMHANDLER(, _ZN6scaler21ExtFuncCallHook_Linux20cPreHookHanlderLinuxEPvS1_)

//    IMPL_ASMHANDLER(Sec, _ZN6scaler21ExtFuncCallHook_Linux23cPreHookHanlderLinuxSecEPvS1_)

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

        __asm__ __volatile__ (
        /**
         * Save environment
         */


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

        //R11 is the only register we can use. Store stack address in it. (%r11)==callerAddr. Later we'll pass this value
        //as a parameter to prehook
        "movq %rsp,%r11\n\t"

        // currsp=oldrsp-152
        //Save Parameter registers and RDI, RSI, RDX, RCX, R8, R9, R10
        "push %rdi\n\t"  // currsp=oldrsp-160
        "push %rsi\n\t"  // currsp=oldrsp-168
        "push %rdx\n\t"  // currsp=oldrsp-176
        "push %rcx\n\t"  // currsp=oldrsp-184
        "push %r8\n\t"   // currsp=oldrsp-192
        "push %r9\n\t"   // currsp=oldrsp-200
        "push %r10\n\t"  // currsp=oldrsp-208
        //Save [XYZ]MM[0-7]
        PUSHXMM(0) // currsp=oldrsp-224
        PUSHXMM(1) // currsp=oldrsp-240
        PUSHXMM(2) // currsp=oldrsp-256
        PUSHXMM(3) // currsp=oldrsp-272
        PUSHXMM(4) // currsp=oldrsp-288
        PUSHXMM(5) // currsp=oldrsp-304
        PUSHXMM(6) // currsp=oldrsp-320
        PUSHXMM(7) // currsp=oldrsp-336
        //todo: Also save YMM0-7 and ZMM0-7
        PUSHYMM(0) // currsp=oldrsp-368
        PUSHYMM(1) // currsp=oldrsp-400
        PUSHYMM(2) // currsp=oldrsp-432
        PUSHYMM(3) // currsp=oldrsp-464
        PUSHYMM(4) // currsp=oldrsp-496
        PUSHYMM(5) // currsp=oldrsp-528
        PUSHYMM(6) // currsp=oldrsp-560
        PUSHYMM(7) // currsp=oldrsp-592
        //Save RBX, RSP, RBP, and R12–R15
        "push %rbx\n\t" // currsp=oldrsp-600
        "push %rsp\n\t" // currsp=oldrsp-608
        "push %rbp\n\t" // currsp=oldrsp-616
        "push %r12\n\t" // currsp=oldrsp-624
        "push %r13\n\t" // currsp=oldrsp-632
        "push %r14\n\t" // currsp=oldrsp-640
        "push %r15\n\t" // currsp=oldrsp-648
        "push %rax\n\t" // currsp=oldrsp-656

        /**
         * Getting PLT entry address and caller address from stack
         */
        "addq $8,%r11\n\t"
        "movq (%r11),%rsi\n\t" //R11 stores callerAddr
        "addq $8,%r11\n\t"
        "movq (%r11),%rdi\n\t" //R11 stores funcID
        "addq $8,%r11\n\t"
        "movq (%r11),%rdx\n\t" //R11 stores fileID

        //size_t fileId (rdx), size_t funcId (rdi), void *callerAddr (rsi)

        /**
         * Pre-Hook
         */
        "call  _ZN6scaler21ExtFuncCallHook_Linux23cPreHookHanlderLinuxSecEmmPv\n\t"
        //Save return value to R11. This is the address of real function parsed by handler.
        //The return address is maybe the real function address. Or a pointer to the pseodoPlt table
        "movq %rax,%r11\n\t"


        /**
        * Restore Registers
        */
        "pop %rax\n\t" // currsp=oldrsp-656
        "pop %r15\n\t" // currsp=oldrsp-640
        "pop %r14\n\t" // currsp=oldrsp-632
        "pop %r13\n\t" // currsp=oldrsp-624
        "pop %r12\n\t" // currsp=oldrsp-616
        "pop %rbp\n\t" // currsp=oldrsp-608
        "pop %rsp\n\t" // currsp=oldrsp-600
        "pop %rbx\n\t" // currsp=oldrsp-592
        POPYMM(7) // currsp=oldrsp-560
        POPYMM(6) // currsp=oldrsp-528
        POPYMM(5) // currsp=oldrsp-496
        POPYMM(4) // currsp=oldrsp-464
        POPYMM(3) // currsp=oldrsp-432
        POPYMM(2) // currsp=oldrsp-400
        POPYMM(1) // currsp=oldrsp-368
        POPYMM(0) // currsp=oldrsp-336
        POPXMM(7) // currsp=oldrsp-320
        POPXMM(6) // currsp=oldrsp-304
        POPXMM(5) // currsp=oldrsp-288
        POPXMM(4) // currsp=oldrsp-272
        POPXMM(3) // currsp=oldrsp-256
        POPXMM(2) // currsp=oldrsp-240
        POPXMM(1) // currsp=oldrsp-224
        POPXMM(0) // currsp=oldrsp-208
        "pop %r10\n\t" // currsp=oldrsp-200
        "pop %r9\n\t" // currsp=oldrsp-192
        "pop %r8\n\t" // currsp=oldrsp-184
        "pop %rcx\n\t" // currsp=oldrsp-176
        "pop %rdx\n\t" // currsp=oldrsp-168
        "pop %rsi\n\t" // currsp=oldrsp-160
        "pop %rdi\n\t" // currsp=oldrsp-152

        // currsp=oldrsp-152

        //Restore rsp to original value
        //"addq $152,%rsp\n\t"
        //        "addq $24,%rsp\n\t"
        //"jmp *%r11\n\t"

        /**
         * Call actual function
         */
        "addq $152,%rsp\n\t"
        "call *%r11\n\t"


        //Save return value to stack
        "push %rax\n\t"
        "push %rdx\n\t"
        PUSHXMM(0)
        PUSHXMM(1)
        PUSHYMM(0)
        PUSHYMM(1)
        //todo: Handle ZMM

        /**
        * Save Environment
        */
        "push %rdi\n\t"
        "push %rsi\n\t"
        "push %rdx\n\t"
        "push %rcx\n\t"
        "push %r8\n\t"
        "push %r9\n\t"
        "push %r10\n\t"
        //Save [XYZ]MM[0-7]
        PUSHXMM(0)
        PUSHXMM(1)
        PUSHXMM(2)
        PUSHXMM(3)
        PUSHXMM(4)
        PUSHXMM(5)
        PUSHXMM(6)
        PUSHXMM(7)
        //todo: Also save YMM0-7 and ZMM0-7
        PUSHYMM(0)
        PUSHYMM(1)
        PUSHYMM(2)
        PUSHYMM(3)
        PUSHYMM(4)
        PUSHYMM(5)
        PUSHYMM(6)
        PUSHYMM(7)
        //Save RBX, RSP, RBP, and R12–R15
        "push %rbx\n\t"
        "push %rsp\n\t"
        "push %rbp\n\t"
        "push %r12\n\t"
        "push %r13\n\t"
        "push %r14\n\t"
        "push %r15\n\t"

        /**
         * Call After Hook
         */
        "call  _ZN6scaler21ExtFuncCallHook_Linux22cAfterHookHanlderLinuxEv\n\t"
        //Save return value to R11. R11 now has the address of caller.
        //"movq %rax,%r11\n\t"

        /**
        * Restore Environment
        */
        "pop %r15\n\t"
        "pop %r14\n\t"
        "pop %r13\n\t"
        "pop %r12\n\t"
        "pop %rbp\n\t"
        "pop %rsp\n\t"
        "pop %rbx\n\t"
        POPXMM(7)
        POPXMM(6)
        POPXMM(5)
        POPXMM(4)
        POPXMM(3)
        POPXMM(2)
        POPXMM(1)
        POPXMM(0)
        POPYMM(7)
        POPYMM(6)
        POPYMM(5)
        POPYMM(4)
        POPYMM(3)
        POPYMM(2)
        POPYMM(1)
        POPYMM(0)
        "pop %r10\n\t"
        "pop %r9\n\t"
        "pop %r8\n\t"
        "pop %rcx\n\t"
        "pop %rdx\n\t"
        "pop %rsi\n\t"
        "pop %rdi\n\t"

        //Restore return value of real function from stack
        POPYMM(1)
        POPYMM(0)
        POPXMM(1)
        POPXMM(0)
        "pop %rdx\n\t"
        "pop %rax\n\t"
        //todo: Handle float return and XMM return


        //Retrun to caller
        "ret\n\t"
        );

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
