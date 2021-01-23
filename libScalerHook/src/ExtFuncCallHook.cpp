

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
//#include "FileTool.h"

extern "C" {
void __attribute__((naked)) asmHookHandlerSec();

void *cPreHookHanlderLinuxSec(void *callerFuncAddr);
}

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


    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    ExtFuncCallHook_Linux *ExtFuncCallHook_Linux::instance = nullptr;
    thread_local bool Hook::SCALER_HOOK_IN_HOOK_HANDLER = false;


    void ExtFuncCallHook_Linux::locateRequiredSecAndSeg() {
        //todo: this function is too long.

        //Get segment info from /proc/self/maps
        //Iterate through libraries

        //pmParser.printPM();

        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            auto &curFileName = iter->first;
            auto &pmEntries = iter->second;
            size_t curFileiD = pmParser.fileIDMap[curFileName];
            auto &curELFImgInfo = elfImgInfoMap[curFileiD];
            curELFImgInfo.fileName = curFileName;

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
                //todo 32 is take
                void *pltAddrInFile = elfParser.getSecContent(pltHdr);
                curELFImgInfo.pltStartAddr = searchBinInMemory(pltAddrInFile, 32,
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

                    assert(curELFImgInfo.pltSecEndAddr);
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

                //Find Base address through _DYNAMICAddr
                //Dl_info info;
                //assert(dladdr(curELFImgInfo._DYNAMICAddr, &info) != 0);

                uint8_t *curBaseAddr = pmParser.fileBaseAddrMap[curFileiD];
                curELFImgInfo.baseAddr = curBaseAddr;

                //printf("curFileName :%s curBaseAddr:%p\n", curFileName.c_str(), curBaseAddr);


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

                    ExtSym newSymbol;
                    newSymbol.symbolName = std::string(curELFImgInfo.dynStrTable + idx);
                    newSymbol.gotTableAddr = reinterpret_cast<void **>(curBaseAddr + curRelaPlt->r_offset);

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


    void *ExtFuncCallHook_Linux::searchBinInMemory(void *secPtrInFile, size_t firstEntrySize,
                                                   const std::vector<PMEntry_Linux> &segments) {

        void *rltAddr = nullptr;

        for (int i = 0; i < segments.size(); ++i) {
            rltAddr = binCodeSearch(segments[i].addrStart, segments[i].length, secPtrInFile, firstEntrySize);
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
                pmParser.fileIDMap["/home/st/Projects/Scaler/cmake-build-debug/tests/libInstallTest.so"]);

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
                curExtSym.funcAddr = *curExtSym.gotTableAddr;

                //Allocate plt table
                auto binCodeArr = fillDestAddr2PseudoPltCode(funcId, curELFImgInfo.pltStartAddr);
                //Copy array
                memcpy(curELFImgInfo.pseudoPlt + 18 * funcId, binCodeArr.data(), 18);
            }

        }

        //todo: Also replace .plt
        //Step4: Replace .plt.sec
        for (int i = 0; i < fileIdToHook.size(); ++i) {
            auto &curELFImgInfo = elfImgInfoMap.at(fileIdToHook[i]);

            for (auto iter = curELFImgInfo.idFuncMap.begin(); iter != curELFImgInfo.idFuncMap.end(); ++iter) {
                auto &funcId = iter->first;

                auto &curSymbol = curELFImgInfo.allExtSymbol.at(funcId);
                size_t symbolFileId = pmParser.findExecNameByAddr(curSymbol.funcAddr);
                curELFImgInfo.realAddrResolved.emplace_back(symbolFileId != i);

                curELFImgInfo.hookedExtSymbol[funcId] = curSymbol;
                curSymbol.funcAddr = *curSymbol.gotTableAddr;
                //pmParser.findExecNameByAddr()

                //printf("%s hooked (ID: %d)\n", curSymbol.symbolName.c_str(), funcId);

                auto binCodeArr = fillDestAddr2HookCode((void *) asmHookHandlerSec);
                //Replace PLT
                //todo: 16 is bin code dependent
                memcpy((uint8_t *) curELFImgInfo.pltSecStartAddr + 16 * funcId, binCodeArr.data(), 16);
            }

        }

    }


    ExtFuncCallHook_Linux::ExtFuncCallHook_Linux() {
        //Expose self to CHookHandler
        __extFuncCallHookPtr = this;
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
        std::vector<uint8_t> binCodeArr = {73, 191, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 215, 104, 144, 144};
        const uint64_t h1 = 0b00000000000000000000000011111111;
        const uint64_t h2 = h1 << 8;
        const uint64_t h3 = h1 << 16;
        const uint64_t h4 = h1 << 24;
        const uint64_t h5 = h1 << 32;
        const uint64_t h6 = h1 << 40;
        const uint64_t h7 = h1 << 48;
        const uint64_t h8 = h1 << 56;

        auto _funcAddr = (ElfW(Addr)) funcAddr;

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

    void ExtFuncCallHook_Linux::updateGotAddr(ELFImgInfo &curELFImgInfo, size_t funcID) {


    }


}


//todo: Currently only pre-hook is implemented. We can't call a function that's not resolved. Have to return now
//todo: A work-around would be don't hook libScalerHook.so 's PLT
//todo: 16 is machine specific
//todo: 18 depends on opCode array
#define IMPL_PREHOOK_HANDLER(suffix, USED_SEC_NAME)
//void *cPreHookHanlderLinux##suffix(int index, void *callerFuncAddr) { \
//    size_t fileId = __extFuncCallHookPtr->findExecNameByAddr(callerFuncAddr); \
//    \
//    auto &pltSec = __extFuncCallHookPtr->fileSecInfoMap.at(fileId).at(scaler::SEC_NAME::USED_SEC_NAME);\
//    auto &realAddrResolved = __extFuncCallHookPtr->realAddrResolved.at(fileId);\
//    auto &hookedAddrs = __extFuncCallHookPtr->hookedAddrs.at(fileId);\
//    auto &curGOT = __extFuncCallHookPtr->fileGotMap.at(fileId);\
//    auto &hookedFuncNames = __extFuncCallHookPtr->hookedFuncNames.at(fileId);\
//    \
//    callerFuncAddr = reinterpret_cast<void *>(uint64_t(callerFuncAddr) - 0xD);\
//    \
//    size_t funcId = ((ElfW(Addr) *) callerFuncAddr - (ElfW(Addr) *) pltSec.startAddr) / 16;\
//    \
//    void *retOriFuncAddr = nullptr;\
//    \
//    if (!realAddrResolved.at(funcId)) {\
//        if (hookedAddrs.at(funcId) != curGOT[funcId]) {\
//            printf("Address is now resolved，replace hookedAddrs with correct value\n");\
//            realAddrResolved.at(funcId) = true;\
//            hookedAddrs.at(funcId) = curGOT[funcId];\
//            retOriFuncAddr = hookedAddrs.at(funcId);\
//        } else {\
//            printf("%s is not initialized, execute orignal PLT code\n", hookedFuncNames.at(funcId).c_str());\
//            retOriFuncAddr = __extFuncCallHookPtr->pseudoPlt + funcId * 18;\
//        }\
//    }\
//    \
//    if (SCALER_HOOK_IN_HOOK_HANDLER) {\
//        return retOriFuncAddr;\
//    }\
//    \
//    SCALER_HOOK_IN_HOOK_HANDLER = true;  \
//    \
//    \
//    SCALER_HOOK_IN_HOOK_HANDLER = false;\
//    return hookedAddrs.at(funcId);\
//}
//
//    extern "C" {
//    IMPL_PREHOOK_HANDLER(Sec, PLT_SEC)
//    IMPL_PREHOOK_HANDLER(, PLT)
//    }
//




//#define IMPL_ASMHANDLER(suffix)\
//void __attribute__((naked)) asmHookHandler##suffix() {\
//    __asm__ __volatile__ (\
//    "popq %r15\n\t" \
//    \
//    "push %rdi\n\t"\
//    "push %rsi\n\t"\
//    "push %rdx\n\t"\
//    "push %rcx\n\t"\
//    "push %r8\n\t"\
//    "push %r9\n\t"\
//    PUSHXMM(0)\
//    PUSHXMM(1)\
//    PUSHXMM(2)\
//    PUSHXMM(3)\
//    PUSHXMM(4)\
//    PUSHXMM(5)\
//    PUSHXMM(6)\
//    PUSHXMM(7)\
//    \
//    "movq %r15,%rsi\n\t" \
//    "movq $1,%rdi\n\t"\
//    "call  cPreHookHanlderLinux##suffix\n\t"\
//    "movq %rax,%r15\n\t"\
//    \
//    POPXMM(7)\
//    POPXMM(6)\
//    POPXMM(5)\
//    POPXMM(4)\
//    POPXMM(3)\
//    POPXMM(2)\
//    POPXMM(1)\
//    POPXMM(0)\
//    "pop %r9\n\t"\
//    "pop %r8\n\t"\
//    "pop %rcx\n\t"\
//    "pop %rdx\n\t"\
//    "pop %rsi\n\t"\
//    "pop %rdi\n\t"\
//    \
//    \
//    "jmp *%r15\n\t"\
//    "ret\n\t"\
//    );\
//}
//
//    extern "C" {
//    IMPL_ASMHANDLER()
//    IMPL_ASMHANDLER(Sec)
//    }

//todo: This function is machine specific
//todo: Binary analysis and support after hook
void __attribute__((naked)) asmHookHandlerSec() {
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
    "call  cPreHookHanlderLinuxSec\n\t"
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


    "jmp *%r14\n\t"
    "ret\n\t"
    );
}

void *cPreHookHanlderLinuxSec(void *callerFuncAddr) {
    //Calculate fileID
    size_t fileId = __extFuncCallHookPtr->pmParser.findExecNameByAddr(callerFuncAddr);
    auto &curElfImgInfo = __extFuncCallHookPtr->elfImgInfoMap[fileId];
    auto &pltSec = curElfImgInfo.pltSecStartAddr;
    auto &realAddrResolved = curElfImgInfo.realAddrResolved;

    callerFuncAddr = reinterpret_cast<void *>((uint8_t *) (callerFuncAddr) - 0xD);
    //todo: 16 is machine specific
    size_t funcId = ((ElfW(Addr)) callerFuncAddr - (ElfW(Addr)) pltSec) / 16;
    auto &curSymbol = curElfImgInfo.hookedExtSymbol.at(funcId);

    void *retOriFuncAddr = nullptr;

    printf("\"%s\" in %s is called\n", curSymbol.symbolName.c_str(),
           __extFuncCallHookPtr->pmParser.idFileMap[fileId].c_str());

    //__extFuncCallHookPtr->pmParser.printPM();

    if (!realAddrResolved[funcId]) {
        void *curAddr = curSymbol.funcAddr;
        void *newAddr = *curSymbol.gotTableAddr;
        if (curAddr != newAddr) {
            printf("\"%s\"'s address is now resolved，update record with the correct address\n",
                   curSymbol.symbolName.c_str());
            //Update address and set it as correct address
            realAddrResolved[funcId] = true;
            curSymbol.funcAddr = newAddr;
            retOriFuncAddr = newAddr;
        } else {
            printf("\"%s\" is not initialized by linker, execute original PLT code\n", curSymbol.symbolName.c_str());
            //Execute original PLT function
            //todo: 18 depends on opCode array
            retOriFuncAddr = curElfImgInfo.pseudoPlt + funcId * 18;
        }
    }

    // if (scaler::Hook::SCALER_HOOK_IN_HOOK_HANDLER) {
    //     return retOriFuncAddr;
    // }

    //Starting from here, we could call external symbols and it won't cause
    //scaler::Hook::SCALER_HOOK_IN_HOOK_HANDLER = true;
    //Currently, I won't load plthook library
    //scaler::Hook::SCALER_HOOK_IN_HOOK_HANDLER = false;
    return *curSymbol.gotTableAddr;
}


#endif
