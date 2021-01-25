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

namespace scaler {

#define PUSHXMM(ArgumentName) \
"subq $16,%rsp\n\t" \
"movdqu  %xmm"#ArgumentName" ,(%rsp)\n\t"

#define POPXMM(ArgumentName) \
"movdqu  (%rsp),%xmm"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"

#define PUSH64bits(ArgumentName) \
"subq $8,%rsp\n\t" \
"movdqu  %"#ArgumentName",(%rsp)\n\t"

#define POP64bits(ArgumentName) \
"movdqu  (%rsp),%"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"


    class Context {
    public:
        //todo: Initialize using maximum stack size
        std::vector<size_t> funcId;
        std::vector<size_t> fileId;
        //Variables used to determine whether it's called by hook handler or not
        bool inHookHanlder = false;
        std::vector<void*> callerAddr;

    };

    thread_local Context curContext;

    //Declare hook handler written in assembly code

//    void __attribute__((naked)) asmHookHandler();

    void __attribute__((naked)) asmHookHandlerSec();

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHook_Linux *ExtFuncCallHook_Linux::instance = nullptr;


    void ExtFuncCallHook_Linux::locateRequiredSecAndSeg() {
        //Get segment info from /proc/self/maps
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            auto &curFileName = iter->first;
            auto &pmEntries = iter->second;
            size_t curFileiD = pmParser.fileIDMap[curFileName];
            auto &curELFImgInfo = elfImgInfoMap[curFileiD];
            curELFImgInfo.filePath = curFileName;

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

                uint8_t *curBaseAddr = pmParser.fileBaseAddrMap[curFileiD];
                curELFImgInfo.baseAddr = curBaseAddr;

                const ElfW(Dyn) *dynsymDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_SYMTAB);

                //Defermine whether current elf file use relative address or absolute address


                if (pmParser.findExecNameByAddr(curBaseAddr + dynsymDyn->d_un.d_ptr) == curFileiD) {
                    //Relative
                } else if (pmParser.findExecNameByAddr((void *) dynsymDyn->d_un.d_ptr) == curFileiD) {
                    //Absolute
                    curBaseAddr = 0;
                    curELFImgInfo.baseAddr = 0;
                } else {
                    assert(false);
                }


                curELFImgInfo.dynSymTable = (const ElfW(Sym) *) (curBaseAddr + dynsymDyn->d_un.d_ptr);


                //printf("dynSymTable:%p %s\n", curELFImgInfo.dynSymTable, info.dli_fname);
                const ElfW(Dyn) *strTabDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRTAB);
                curELFImgInfo.dynStrTable = (const char *) (curBaseAddr + strTabDyn->d_un.d_ptr);


                const ElfW(Dyn) *strSizeDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_STRSZ);
                curELFImgInfo.dynStrSize = strSizeDyn->d_un.d_val;

                ElfW(Dyn) *relaPltDyn = findDynEntryByTag(curELFImgInfo._DYNAMICAddr, DT_JMPREL);
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

                    ExtSymInfo newSymbol;
                    newSymbol.symbolName = std::string(curELFImgInfo.dynStrTable + idx);
                    newSymbol.gotEntry = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

                    curELFImgInfo.idFuncMap[i] = newSymbol.symbolName;
                    curELFImgInfo.funcIdMap[newSymbol.symbolName] = i;
                    curELFImgInfo.allExtSymbol[i] = newSymbol;

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

    void ExtFuncCallHook_Linux::install() {
        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

        //Step2: Change every plt table memory into writeable
        MemoryTool_Linux *memTool = MemoryTool_Linux::getInst();

        //Loop through every linked file
        for (auto iter = elfImgInfoMap.begin(); iter != elfImgInfoMap.end(); ++iter) {
            auto &curELFImgInfo = iter->second;
            //.plt
            memTool->adjustMemPerm(curELFImgInfo.pltStartAddr, curELFImgInfo.pltEndAddr,
                                   PROT_READ | PROT_WRITE | PROT_EXEC);
            //.plt.sec
            if (curELFImgInfo.pltSecStartAddr)
                memTool->adjustMemPerm(curELFImgInfo.pltSecStartAddr, curELFImgInfo.pltSecEndAddr,
                                       PROT_READ | PROT_WRITE | PROT_EXEC);
        }

        //Step3: Decide which to hook
        //todo: Implement API to help user specify which function to hook. Here, I'll only hook current executable.

        std::vector<int> fileIdToHook;
        fileIdToHook.emplace_back(0);
        fileIdToHook.emplace_back(
                pmParser.fileIDMap.at(
                        "/home/st/Projects/Scaler/cmake-build-debug/unittests/libscalerhook/libInstallTest.so"));

        //Step3: Build pseodo PLT
        for (int i = 0; i < fileIdToHook.size(); ++i) {
            auto &curELFImgInfo = elfImgInfoMap.at(fileIdToHook[i]);

            //Malloc mem area for pseodo plt
            //todo: 18 is related to binary code. I should use a global config file to store it.
            curELFImgInfo.pseudoPlt = (uint8_t *) malloc(curELFImgInfo.allExtSymbol.size() * 18);

            for (auto iter = curELFImgInfo.idFuncMap.begin(); iter != curELFImgInfo.idFuncMap.end(); ++iter) {
                auto &funcId = iter->first;
                auto &curExtSym = curELFImgInfo.allExtSymbol.at(funcId);
                //Resole current address
                curExtSym.addr = *curExtSym.gotEntry;

                //Allocate plt table
                auto binCodeArr = fillDestAddr2PseudoPltCode(funcId, curELFImgInfo.pltStartAddr);
                //Copy array
                memcpy(curELFImgInfo.pseudoPlt + 18 * funcId, binCodeArr.data(), 18);
            }

        }

        //Step4: Replace .plt.sec
        for (int i = 0; i < fileIdToHook.size(); ++i) {
            auto &curELFImgInfo = elfImgInfoMap.at(fileIdToHook[i]);

            for (auto iter = curELFImgInfo.idFuncMap.begin(); iter != curELFImgInfo.idFuncMap.end(); ++iter) {
                auto &funcId = iter->first;

                auto &curSymbol = curELFImgInfo.allExtSymbol.at(funcId);
                size_t symbolFileId = pmParser.findExecNameByAddr(curSymbol.addr);
                curELFImgInfo.realAddrResolved.emplace_back(symbolFileId != i);

                curELFImgInfo.hookedExtSymbol[funcId] = curSymbol;

                printf("%s in %s hooked\n", curSymbol.symbolName.c_str(), curELFImgInfo.filePath.c_str());
                //Check if current address has already been hooked
                auto binCodeArr = fillDestAddr2HookCode((void *) asmHookHandlerSec);

                //Replace PLT.SEC
                //todo: 16 is bin code dependent
                memcpy((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * funcId, binCodeArr.data(), 16);
                binCodeArr.clear();

                //binCodeArr = fillDestAddr2HookCode((void *) asmHookHandler);
                //Replace PLT
                //memcpy((uint8_t *) curELFImgInfo.pltStartAddr + 16 * funcId, binCodeArr.data(), 16);
            }

        }

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
        //mov  r15, funcAddr
        //call r15

        //After calling r15. The address of next instruction will be on stack. The hander will pop this address and
        //Calculating function ID by comparing it with the starting address of .plt/.plt.sec address

        std::vector<uint8_t> binCodeArr = {73, 191, 0, 0, 0, 0, 0, 0, 0, 0, 65, 255, 215, 104, 144, 144};

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
        std::vector<uint8_t> binCodeArr = {104, 00, 00, 00, 00, 73, 191, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 231};

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

#define IMPL_CHANDLER(SUFFIX, SEC_START_ADDR_VAR) \
    void *ExtFuncCallHook_Linux::cPreHookHanlderLinux##SUFFIX(void *pltEntryAddr,void* callerAddr) {\
        curContext.callerAddr.emplace_back(callerAddr);                                          \
        auto &_this = ExtFuncCallHook_Linux::instance;\
        auto fileId=_this->pmParser.findExecNameByAddr(pltEntryAddr);              \
        auto &curElfImgInfo = _this->elfImgInfoMap[fileId];\
        auto &SEC_START_ADDR_VAR = curElfImgInfo.SEC_START_ADDR_VAR;\
        auto &realAddrResolved = curElfImgInfo.realAddrResolved;\
        \
        pltEntryAddr = reinterpret_cast<void *>((uint8_t *) (pltEntryAddr) - 0xD);\
        auto funcId=((ElfW(Addr)) pltEntryAddr - (ElfW(Addr)) SEC_START_ADDR_VAR) / 16;\
        auto &curSymbol = curElfImgInfo.hookedExtSymbol.at(funcId);                  \
        \
        void *retOriFuncAddr = nullptr; \
        \
        if (!realAddrResolved[funcId]) {\
            void *curAddr = curSymbol.addr;\
            void *newAddr = *curSymbol.gotEntry;\
            if (curAddr == newAddr) {\
                printf("\"%s\" is not initialized by linker, execute original PLT code\n",\
                curSymbol.symbolName.c_str());\
                retOriFuncAddr = curElfImgInfo.pseudoPlt + funcId * 18;\
            } else {\
                printf("\"%s\"'s address is now resolved，update record with the correct address\n",\
                curSymbol.symbolName.c_str());\
                \
                realAddrResolved[funcId] = true; \
                curSymbol.addr = newAddr; \
                retOriFuncAddr = newAddr;\
            }\
        }\
        if (curContext.inHookHanlder) {\
            return retOriFuncAddr;\
        }\
        \
        curContext.inHookHanlder = true;\
        printf("[Pre Hook] File:%s, Func: %s\n", _this->pmParser.idFileMap[fileId].c_str(),  curElfImgInfo.idFuncMap.at(funcId).c_str());\
                                                  \
                                                  \
        curContext.fileId.emplace_back(fileId);          \
        curContext.funcId.emplace_back(funcId);          \
                                                  \
                                                  \
        curContext.inHookHanlder = false;\
        return *curSymbol.gotEntry;\
    }


    IMPL_CHANDLER(, pltStartAddr)

    IMPL_CHANDLER(Sec, pltSecStartAddr)

    /**
    void *ExtFuncCallHook_Linux::cPreHookHanlderLinuxSec(void *callerFuncAddr) {
        //Calculate fileID
        auto &_this = ExtFuncCallHook_Linux::instance;
        auto fileId = _this->pmParser.findExecNameByAddr(callerFuncAddr);
        curContext.fileId.emplace_back(fileId);
        auto &curElfImgInfo = _this->elfImgInfoMap[fileId];
        auto &pltSec = curElfImgInfo.pltSecStartAddr;
        auto &realAddrResolved = curElfImgInfo.realAddrResolved;

        callerFuncAddr = reinterpret_cast<void *>((uint8_t *) (callerFuncAddr) - 0xD);
        //todo: 16 is machine specific
        size_t funcId = ((ElfW(Addr)) callerFuncAddr - (ElfW(Addr)) pltSec) / 16;
        curContext.funcId.emplace_back(funcId);
        auto &curSymbol = curElfImgInfo.hookedExtSymbol.at(funcId);

        void *retOriFuncAddr = nullptr;

        //printf("\"%s\" in %s is called\n", curSymbol.symbolName.c_str(),
        //       _this->pmParser.idFileMap[fileId].c_str());

        //_this->pmParser.printPM();



        if (!realAddrResolved[funcId]) {
            void *curAddr = curSymbol.addr;
            void *newAddr = *curSymbol.gotEntry;
            if (curAddr == newAddr) {
                printf("\"%s\" is not initialized by linker, execute original PLT code\n",
                       curSymbol.symbolName.c_str());
                //Execute original PLT function
                //todo: 18 depends on opCode array
                retOriFuncAddr = curElfImgInfo.pseudoPlt + funcId * 18;
            } else {
                printf("\"%s\"'s address is now resolved，update record with the correct address\n",
                       curSymbol.symbolName.c_str());
                //Update address and set it as correct address
                realAddrResolved[funcId] = true;
                curSymbol.addr = newAddr;
                retOriFuncAddr = newAddr;
            }
        }

        if (curContext.inHookHanlder) {
            return retOriFuncAddr;
        }

        //Starting from here, we could call external symbols and it won't cause
        curContext.inHookHanlder = true;
        printf("[Pre Hook] File:%s, Func: %s\n", _this->pmParser.idFileMap[fileId].c_str(),
               curElfImgInfo.idFuncMap.at(funcId).c_str());\
        //Currently, I won't load plthook library
//        _this->fileId1.push_back(fileId);
//        _this->funcId1.push_back(funcId);

        curContext.inHookHanlder = false;
        return *curSymbol.gotEntry;
    }
**/

    void *ExtFuncCallHook_Linux::cAfterHookHanlderLinux() {
        auto &_this = ExtFuncCallHook_Linux::instance;

        size_t fileId = curContext.fileId.at(curContext.fileId.size() - 1);
        curContext.fileId.pop_back();

        auto &curELFImgInfo = _this->elfImgInfoMap.at(fileId);

        auto &fileName = curELFImgInfo.filePath;

        size_t funcId = curContext.funcId.at(curContext.funcId.size() - 1);
        curContext.funcId.pop_back();
        auto &funcName = curELFImgInfo.idFuncMap.at(funcId);

        printf("[After Hook] File:%s, Func: %s\n", fileName.c_str(), funcName.c_str());

        void* callerAddr = curContext.callerAddr.at(curContext.callerAddr.size() - 1);
        curContext.callerAddr.pop_back();

        return callerAddr;
    }

//todo: This function is machine specific
//todo: Binary analysis and support after hook

#define IMPL_ASMHANDLER(SUFFIX, FUNC_NAME)\
    void __attribute__((naked)) asmHookHandler##SUFFIX() {\
        __asm__ __volatile__ (\
        "popq %r14\n\t" \
        "popq %r13\n\t" \
        \
        \
        "push %rdi\n\t"\
        "push %rsi\n\t"\
        "push %rdx\n\t"\
        "push %rcx\n\t"\
        "push %r8\n\t"\
        "push %r9\n\t"\
        PUSHXMM(0)\
        PUSHXMM(1)\
        PUSHXMM(2)\
        PUSHXMM(3)\
        PUSHXMM(4)\
        PUSHXMM(5)\
        PUSHXMM(6)\
        PUSHXMM(7)\
        PUSHXMM(8)\
        \
        "movq %r14,%rdi\n\t"\
        "movq %r13,%rsi\n\t"\
        "call  "#FUNC_NAME"\n\t" \
        "movq %rax,%r14\n\t" \
        \
        POPXMM(8)\
        POPXMM(7)\
        POPXMM(6)\
        POPXMM(5)\
        POPXMM(4)\
        POPXMM(3)\
        POPXMM(2)\
        POPXMM(1)\
        POPXMM(0)\
        "pop %r9\n\t"\
        "pop %r8\n\t"\
        "pop %rcx\n\t"\
        "pop %rdx\n\t"\
        "pop %rsi\n\t"\
        "pop %rdi\n\t"\
        \
        "call *%r14\n\t"\
        \
        "call  _ZN6scaler21ExtFuncCallHook_Linux22cAfterHookHanlderLinuxEv\n\t"  \
        "movq %rax,%r13\n\t" \
        "jmp *%r13\n\t"\
        );\
    }

    //I used to put cHookHandler out of scaler namespace
    //However, cHookHandler needs to access variables defined in ExtHook_Linux
    //If I use friend function, it cannot be called by Assembly code.
    //To make code more elegant, I finally put everything in scaler namespace.
    //The cost is I have to find compiled function name once
    IMPL_ASMHANDLER(, _ZN6scaler21ExtFuncCallHook_Linux20cPreHookHanlderLinuxEPvS1_)

    IMPL_ASMHANDLER(Sec, _ZN6scaler21ExtFuncCallHook_Linux23cPreHookHanlderLinuxSecEPvS1_)

/**
 * Source code version for #define IMPL_ASMHANDLER
 * We can't add comments to a macro
 */
/**    void __attribute__((naked)) asmHookHandlerSec() {
    __asm__ __volatile__ (
    "popq %r14\n\t" //Save caller address from PLT

    //Save environment
    "push %rdi\n\t"
    "push %rsi\n\t"
    "push %rdx\n\t"
    "push %rcx\n\t"
    "push %r8\n\t"
    "push %r9\n\t"
    PUSHXMM(0)
    PUSHXMM(1)
    PUSHXMM(2)
    PUSHXMM(3)
    PUSHXMM(4)
    PUSHXMM(5)
    PUSHXMM(6)
    PUSHXMM(7)

    "movq %r14,%rdi\n\t" //Pass PLT call address to cPreHookHanlderLinux
    //cPreHookHanlderLinuxSec needs access to ExtFuncCallHook_Linux
    "call  _ZN6scaler21ExtFuncCallHook_Linux23cPreHookHanlderLinuxSecEPv\n\t" //Calling cPreHookHanlderLinuxSec
    "movq %rax,%r14\n\t"

    //Restore environment
    POPXMM(7)
    POPXMM(6)
    POPXMM(5)
    POPXMM(4)
    POPXMM(3)
    POPXMM(2)
    POPXMM(1)
    POPXMM(0)
    "pop %r9\n\t"
    "pop %r8\n\t"
    "pop %rcx\n\t"
    "pop %rdx\n\t"
    "pop %rsi\n\t"
    "pop %rdi\n\t"

    "call *%r14\n\t"

    "call  cHookHanlderAfter\n\t"

    "jmp *%r13\n\t"
    );
} **/

}


#endif
