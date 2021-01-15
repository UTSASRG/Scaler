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

        size_t counter = 0;
        //Iterate through libraries
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            //Save file name to id table
            fileNameIDMap[iter->first] = counter;

            //Open corresponding ELF file
            ELFParser elfParser(iter->first);

            try {
                //Some segment may throw exception in parse. So we won't store invalid segments into fileSecMap
                elfParser.parse();

                filePltNameMap[counter] = elfParser.relaFuncName;

                auto &curFile = fileSecMap[counter];

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
                recordGOT(curPLTGot, counter);

            } catch (const ScalerException &e) {
                std::stringstream ss;
                ss << "Hook Failed for \"" << elfParser.elfPath << "\" because " << e.info;
                fprintf(stderr, "%s\n", ss.str().c_str());
            }
            counter++;

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
        //Step2: Change every plt location to writeable
        for (auto iterFile = fileSecMap.begin(); iterFile != fileSecMap.end(); ++iterFile) {
            for (auto iterSec = iterFile->second.begin(); iterSec != iterFile->second.end(); ++iterSec) {
                auto &curSecInfo = iterSec->second;
                adjustMemPermission(curSecInfo.startAddr, curSecInfo.endAddr, PROT_READ | PROT_WRITE | PROT_EXEC);
            }
        }
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
        auto &curGOTTbl = fileGotMap[fileId];
        auto itemSize = gotShHdr.secTotalSize / gotShHdr.itemSize;
        for (int i = 0; i < itemSize; ++i) {
            curGOTTbl.emplace_back((void *) (ElfW(Addr)(gotShHdr.startAddr) + i * itemSize));
        }
    }

}


extern "C" {
//todo: Currently only pre-hook is implemented. We can't call a function that's not resolved. Have to return now
//todo: A work-around would be don't hook libScalerHook.so 's PLT
void *cPreHookHanlderSec(int index, void *callerFuncAddr) {
    size_t fileId = __extFuncCallHookPtr->findExecNameByAddr(callerFuncAddr);

    auto &pltSec = __extFuncCallHookPtr->fileSecMap[fileId][scaler::SEC_NAME::PLT_SEC];
    auto &realAddrResolved = __extFuncCallHookPtr->realAddrResolved[fileId];
    auto &hookedAddrs = __extFuncCallHookPtr->hookedAddrs[fileId];
    auto &curGOT = __extFuncCallHookPtr->fileGotMap[fileId];
    auto &hookedNames = __extFuncCallHookPtr->hookedNames[fileId];


    callerFuncAddr = reinterpret_cast<void *>(uint64_t(callerFuncAddr) - 0xD);
    //todo: 16 is machine specific
    size_t funcId = ((ElfW(Addr) *) callerFuncAddr - (ElfW(Addr) *) pltSec.startAddr) / 16;

    void *retOriFuncAddr = nullptr;


    if (!realAddrResolved[funcId]) {
        if (hookedAddrs[funcId] != curGOT[funcId]) {
            printf("Address is now resolved，replace hookedAddrs with correct value\n");
            //Update address and set it as correct address
            realAddrResolved[funcId] = true;
            hookedAddrs[funcId] = curGOT[funcId];
            retOriFuncAddr = hookedAddrs[funcId];
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

    /**
     * Starting from here, we could call external symbols and it won't cause
     */
    SCALER_HOOK_IN_HOOK_HANDLER = true;
    //Currently, I won't load plthook library
    SCALER_HOOK_IN_HOOK_HANDLER = false;
    return hookedAddrs[funcId];
}
}

