#include <util/hook/ExtFuncCallHook.hh>
#include <util/tool/PMParser.h>
#include <cstdint>
#include <util/tool/ElfParser.h>
#include <util/tool/FileTool.h>
#include <exceptions/ScalerException.h>
#include <sys/mman.h>
#include <algorithm>

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


#define ALIGN_ADDR(addr, page_size) (ElfW(Addr) *) ((size_t) (addr) / page_size * page_size)

    ExtFuncCallHook *ExtFuncCallHook::instance = nullptr;

    void ExtFuncCallHook::locSectionInMem() {

        //Get segment info from /proc/self/maps
        PMParser pmParser;
        pmParser.parsePMMap();

        recordFileSecMap(pmParser);

        size_t fileID = 0;
        //Iterate through libraries
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            //Save file name to id table
            fileIDMap[iter->first] = fileID;

            //Open corresponding ELF file
            ELFParser elfParser(iter->first);

            try {
                //Some segment may throw exception in parse. So we won't store invalid segments into fileSecMap
                elfParser.parse();

                fileExtFuncNameMap[fileID] = elfParser.relaFuncName;

                auto &curFile = fileSecMap.at(fileID);

                auto &curPLT = curFile[SEC_NAME::PLT];
                curPLT.startAddr = searchSecLoadingAddr(".plt", elfParser, iter->second);

                auto &curPLTSec = curFile[SEC_NAME::PLT_SEC];
                curPLTSec.startAddr = searchSecLoadingAddr(".plt.sec", elfParser, iter->second);

                auto &curPLTGot = curFile[SEC_NAME::GOT];
                curPLTGot.startAddr = searchSecLoadingAddr(".plt.got", elfParser, iter->second);

                //Make sure every section is found
                for (auto iter1 = curFile.begin();
                     iter1 != curFile.end(); ++iter1) {
                    if (iter1->second.startAddr == nullptr) {
                        std::stringstream ss;
                        ss << "Section " << iter1->first << " not found in " << iter->first;
                        throwScalerException(ss.str().c_str());
                    }
                }

                //We already have the starting address, let's calculate the end address
                curPLT.endAddr =
                        (char *) curPLT.startAddr + elfParser.getSecLength(".plt");
                auto *curPLTShHdr = elfParser.getSecHdrByName(".plt");
                curPLT.itemSize = curPLTShHdr->sh_entsize;
                curPLT.secTotalSize = curPLTShHdr->sh_size;

                curPLTSec.endAddr =
                        (char *) curPLTSec.startAddr + elfParser.getSecLength(".plt.sec");
                auto *curPLTSecShHdr = elfParser.getSecHdrByName(".plt.sec");
                curPLTSec.itemSize = curPLTSecShHdr->sh_entsize;
                curPLTSec.secTotalSize = curPLTSecShHdr->sh_size;

                curPLTGot.endAddr =
                        (char *) curPLTGot.startAddr + elfParser.getSecLength(".plt.got");
                auto *curPLTGotShHdr = elfParser.getSecHdrByName(".plt.got");
                curPLTGot.itemSize = curPLTGotShHdr->sh_entsize;
                curPLTGot.secTotalSize = curPLTGotShHdr->sh_size;

                //We've got GOT table, store it
                recordGOT(curPLTGot, fileID);

            } catch (const ScalerException &e) {
                std::stringstream ss;
                ss << "Hook Failed for \"" << elfParser.elfPath << "\" because " << e.info;
                fprintf(stderr, "%s\n", ss.str().c_str());
            }
            fileID++;
        }
    }

    void *ExtFuncCallHook::searchSecLoadingAddr(std::string secName, ELFParser &elfParser,
                                                const std::vector<PMEntry> &segments) {
        //Read the section specified by secName from ELF file.
        void *secPtr = elfParser.getSecPtr(secName);
        if (!secPtr) {
            std::stringstream ss;
            ss << "Cannot find section " << secName << " in " << elfParser.elfPath;
            throwScalerException(ss.str().c_str());
        }

        void *rltAddr = nullptr;

        //Match the first 20 bytes
        for (int i = 0; i < segments.size(); ++i) {
            if (segments[i].isE) {
                //Only check executable secitons
                rltAddr = binarySearch(segments[i].addrStart, segments[i].length, secPtr, 20);
                if (rltAddr)
                    break;
            }
        }
        return rltAddr;
    }

    void ExtFuncCallHook::install() {
        //Step1: Locating table in memory
        locSectionInMem();
        //Step2: Change every plt table memory into writeable
        for (auto iterFile = fileSecMap.begin(); iterFile != fileSecMap.end(); ++iterFile) {
            for (auto iterSec = iterFile->second.begin(); iterSec != iterFile->second.end(); ++iterSec) {
                auto &curSecInfo = iterSec->second;
                adjustMemPermission(curSecInfo.startAddr, curSecInfo.endAddr, PROT_READ | PROT_WRITE | PROT_EXEC);
            }
        }
        //Step3: Decide which to hook



        //Step3: Replace PLT

    }

    void ExtFuncCallHook::adjustMemPermission(void *startPtr, void *endPtr, int prem) {
        size_t pageSize = sysconf(_SC_PAGESIZE);
        void *alignedStartPtr = ALIGN_ADDR(startPtr, pageSize);
        void *alignedEndPtr = ALIGN_ADDR(endPtr, pageSize);

        if (mprotect(alignedStartPtr, (ElfW(Addr) *) alignedEndPtr - (ElfW(Addr) *) alignedStartPtr, prem) != 0) {
            std::stringstream ss;
            ss << "Could not change the process memory permission at " << alignedStartPtr << " - " << alignedEndPtr;
            throwScalerException(ss.str().c_str());
        }
    }

    size_t ExtFuncCallHook::findExecNameByAddr(void *addr) {
        //Binary search segAddrFileMap
        long lo = 0, hi = segAddrFileMap.size(), md;
        //[lo,hi) to prevent underflow
        while (lo < hi - 1) {
            md = (lo + hi) / 2;
            if (segAddrFileMap[md].startAddr > addr) {
                hi = md;
            } else {
                lo = md + 1;
            }
        }
        return lo;
    }

    void ExtFuncCallHook::recordFileSecMap(PMParser &pmParser) {
        for (auto iterFile = pmParser.procMap.begin(); iterFile != pmParser.procMap.end(); ++iterFile) {
            for (auto iterSeg = iterFile->second.begin(); iterSeg != iterFile->second.end(); ++iterSeg) {
                SegInfo newEntry;
                newEntry.fileName = iterFile->first;
                newEntry.startAddr = iterSeg->addrStart;
                newEntry.endAddr = iterSeg->addrEnd;
                segAddrFileMap.emplace_back(newEntry);
            }
        }
        //Sort by start address
        std::sort(segAddrFileMap.begin(), segAddrFileMap.end(), [](const SegInfo &lhs, const SegInfo &rhs) {
            return lhs.startAddr < rhs.startAddr;
        });
    }

    ExtFuncCallHook::ExtFuncCallHook() {
        //Expose self to CHookHandler
        __extFuncCallHookPtr = this;
    }

    ExtFuncCallHook *ExtFuncCallHook::getInst() {
        if (!instance)
            instance = new ExtFuncCallHook();
        return instance;
    }

    void ExtFuncCallHook::recordGOT(SecInfo &gotShHdr, size_t fileId) {
        //auto &curGOTTbl = fileGotMap.at(fileId);
        auto itemSize = gotShHdr.secTotalSize / gotShHdr.itemSize;
        for (int i = 0; i < itemSize; ++i) {
            fileGotMap.at(fileId).emplace_back((void *) (ElfW(Addr)(gotShHdr.startAddr) + i * itemSize));
        }
    }

    std::vector<uint8_t> ExtFuncCallHook::fillDestAddr2HookCode(void *funcAddr) {
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

    std::vector<uint8_t> ExtFuncCallHook::fillDestAddr2PseudoPltCode(size_t funcId, void *funcAddr) {
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

    void *ExtFuncCallHook::getFuncAddrFromGOTByName(size_t fileId, std::string name) {
        size_t funcID = fileExtFuncNameMap.at(fileId).at(name);
        printf("File Size: %zu\n",fileGotMap.at(fileId).size());
        return fileGotMap.at(fileId)[funcID];
    }

}


//todo: Currently only pre-hook is implemented. We can't call a function that's not resolved. Have to return now
//todo: A work-around would be don't hook libScalerHook.so 's PLT
//todo: 16 is machine specific
//todo: 18 depends on opCode array
#define IMPL_PREHOOK_HANDLER(suffix, USED_SEC_NAME) \
void *cPreHookHanlder##suffix(int index, void *callerFuncAddr) { \
    size_t fileId = __extFuncCallHookPtr->findExecNameByAddr(callerFuncAddr); \
    \
    auto &pltSec = __extFuncCallHookPtr->fileSecMap.at(fileId).at(scaler::SEC_NAME::USED_SEC_NAME);\
    auto &realAddrResolved = __extFuncCallHookPtr->realAddrResolved.at(fileId);\
    auto &hookedAddrs = __extFuncCallHookPtr->hookedAddrs.at(fileId);\
    auto &curGOT = __extFuncCallHookPtr->fileGotMap.at(fileId);\
    auto &hookedFuncNames = __extFuncCallHookPtr->hookedFuncNames.at(fileId);\
    \
    callerFuncAddr = reinterpret_cast<void *>(uint64_t(callerFuncAddr) - 0xD);\
    \
    size_t funcId = ((ElfW(Addr) *) callerFuncAddr - (ElfW(Addr) *) pltSec.startAddr) / 16;\
    \
    void *retOriFuncAddr = nullptr;\
    \
    if (!realAddrResolved.at(funcId)) {\
        if (hookedAddrs.at(funcId) != curGOT[funcId]) {\
            printf("Address is now resolved，replace hookedAddrs with correct value\n");\
            realAddrResolved.at(funcId) = true;\
            hookedAddrs.at(funcId) = curGOT[funcId];\
            retOriFuncAddr = hookedAddrs.at(funcId);\
        } else {\
            printf("%s is not initialized, execute orignal PLT code\n", hookedFuncNames.at(funcId).c_str());\
            retOriFuncAddr = __extFuncCallHookPtr->pseudoPlt + funcId * 18;\
        }\
    }\
    \
    if (SCALER_HOOK_IN_HOOK_HANDLER) {\
        return retOriFuncAddr;\
    }\
    \
    SCALER_HOOK_IN_HOOK_HANDLER = true;  \
    \
    \
    SCALER_HOOK_IN_HOOK_HANDLER = false;\
    return hookedAddrs.at(funcId);\
}

extern "C" {
IMPL_PREHOOK_HANDLER(Sec, PLT_SEC)
IMPL_PREHOOK_HANDLER(, PLT)
}
/**
void *cPreHookHanlderSec(int index, void *callerFuncAddr) {
    size_t fileId = __extFuncCallHookPtr->findExecNameByAddr(callerFuncAddr);

    auto &pltSec = __extFuncCallHookPtr->fileSecMap.at(fileId).at(scaler::SEC_NAME::PLT_SEC);
    auto &realAddrResolved = __extFuncCallHookPtr->realAddrResolved.at(fileId);
    auto &hookedAddrs = __extFuncCallHookPtr->hookedAddrs.at(fileId);
    auto &curGOT = __extFuncCallHookPtr->fileGotMap.at(fileId);
    auto &hookedNames = __extFuncCallHookPtr->hookedNames[fileId];


    callerFuncAddr = reinterpret_cast<void *>(uint64_t(callerFuncAddr) - 0xD);
    //todo: 16 is machine specific
    size_t funcId = ((ElfW(Addr) *) callerFuncAddr - (ElfW(Addr) *) pltSec.startAddr) / 16;

    void *retOriFuncAddr = nullptr;


    if (!realAddrResolved.at(funcId)) {
        if (hookedAddrs.at(funcId) != curGOT[funcId]) {
            printf("Address is now resolved，replace hookedAddrs with correct value\n");
            //Update address and set it as correct address
            realAddrResolved.at(funcId) = true;
            hookedAddrs.at(funcId) = curGOT[funcId];
            retOriFuncAddr = hookedAddrs.at(funcId);
        } else {
            printf("%s is not initialized, execute orignal PLT code\n", hookedNames[funcId].c_str());
            //Execute original PLT function
            //todo: 18 depends on opCode array
            retOriFuncAddr = __extFuncCallHookPtr->pseudoPlt + funcId * 18;
        }
    }

    if (SCALER_HOOK_IN_HOOK_HANDLER) {
        return retOriFuncAddr;
    }

    //Starting from here, we could call external symbols and it won't cause
    SCALER_HOOK_IN_HOOK_HANDLER = true;
    //Currently, I won't load plthook library
    SCALER_HOOK_IN_HOOK_HANDLER = false;
    return hookedAddrs.at(funcId);
}
**/

#define IMPL_ASMHANDLER(suffix)\
void __attribute__((naked)) asmHookHandler##suffix() {\
    __asm__ __volatile__ (\
    "popq %r15\n\t" \
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
    \
    "movq %r15,%rsi\n\t" \
    "movq $1,%rdi\n\t"\
    "call  cPreHookHanlder##suffix\n\t"\
    "movq %rax,%r15\n\t"\
    \
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
    \
    "jmp *%r15\n\t"\
    "ret\n\t"\
    );\
}

extern "C" {
IMPL_ASMHANDLER()
IMPL_ASMHANDLER(Sec)
}


//todo: This function is machine specific
//todo: Binary analysis and support after hook
/**
void __attribute__((naked)) asmHookHandlerSec() {
    __asm__ __volatile__ (
    "popq %r15\n\t" //Save caller address from PLT

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

    "movq %r15,%rsi\n\t" //Pass PLT call address to cPreHookHanlder
    "movq $1,%rdi\n\t"
    "call  cPreHookHanlderSec\n\t"
    "movq %rax,%r15\n\t"

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


    "jmp *%r15\n\t"
    "ret\n\t"
    );
}
**/


