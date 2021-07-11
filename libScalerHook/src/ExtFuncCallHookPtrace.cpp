#ifdef __linux

#include <cstdint>
#include <sys/mman.h>
#include <algorithm>
#include <cassert>
#include <elf.h>
#include <set>
#include <immintrin.h>
#include <thread>
#include <utility>
#include <util/hook/ExtFuncCallHookPtrace.h>
#include <exceptions/ScalerException.h>
#include <util/tool/MemToolPtrace.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>

namespace scaler {

    //todo: ElfW is not correct. Because types are determined by the type of ELF file
    //todo: rather than the type of the machine
    //Initialize instance variable to nullptr;
    ExtFuncCallHookPtrace *ExtFuncCallHookPtrace::instance = nullptr;


    void ExtFuncCallHookPtrace::install(Hook::SYMBOL_FILTER filterCallB) {
        int wait_status;
        //Wait for child to stop on its first instruction
        wait(&wait_status);
        //todo: use debuglog and exceptions
        perror("first wait result");

        memTool = MemoryToolPtrace::getInst(pmParser);

        //Step1: Locating table in memory
        locateRequiredSecAndSeg();

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

        //Step6: Replace PLT table, jmp to dll function
        for (auto &curSymbol:symbolToHook) {
            //todo: we only use one of them. If ,plt.sec exists, hook .plt.sec rather than plt
            recordPltCode(curSymbol);

            recordPltSecCode(curSymbol);

            //todo: Add logic to determine whether hook .plt or .plt.sec. Currently only hook .plt.sec
            instrumentPltSecCode(curSymbol);
        }

        debuggerLoop();
    }

    ExtFuncCallHookPtrace::ExtFuncCallHookPtrace(pid_t childPID)
            : pmParser(childPID), ExtFuncCallHook_Linux(pmParser, *MemoryToolPtrace::getInst(pmParser)) {
        this->childPID = childPID;
    }

    ExtFuncCallHookPtrace *ExtFuncCallHookPtrace::getInst(pid_t childPID) {
        if (!instance)
            instance = new ExtFuncCallHookPtrace(childPID);
        return instance;
    }

    void ExtFuncCallHookPtrace::uninstall() {
    }


    ExtFuncCallHookPtrace::~ExtFuncCallHookPtrace() {

    }

    void ExtFuncCallHookPtrace::parseRelaSymbol(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo, size_t curFileID) {
        //Copy memory from other process to this process. We don't need to worry about the original pointer, because those memory are allocated by the OS.
        //ELF file segments will also be automatically freed after ELFParser is deconstructed.

        //1.copy .plt.rela contents to current process
        curELFImgInfo.relaPlt = static_cast<ElfW(Rela) *>(pmParser.readProcMem(curELFImgInfo.relaPlt,
                                                                               curELFImgInfo.relaPltCnt *
                                                                               sizeof(ElfW(Rela))));

        size_t maxDynSymId = findDynSymTblSize(curELFImgInfo);

        //2.copy dynamic string table to current process
        curELFImgInfo.dynSymTable = reinterpret_cast<const Elf64_Sym *>(pmParser.readProcMem(
                (void *) curELFImgInfo.dynSymTable,
                (maxDynSymId + 1) *
                sizeof(ElfW(Sym))));

        curELFImgInfo.dynStrTable = static_cast<const char *>(pmParser.readProcMem((void *) curELFImgInfo.dynStrTable,
                                                                                   curELFImgInfo.dynStrSize));

        //Copy other process memory to this memory
        ExtFuncCallHook_Linux::parseRelaSymbol(curELFImgInfo, curFileID);

        //Deallocate memory
        free(curELFImgInfo.relaPlt);
        free((void *) curELFImgInfo.dynSymTable);
        free((void *) curELFImgInfo.dynStrTable);
    }

    size_t ExtFuncCallHookPtrace::findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo) {
        ssize_t maxID = -1;
        //Since we don't know the size of .dynsym table, we have to use relocation table to figure it out.
        for (size_t i = 0; i < curELFImgInfo.relaPltCnt; ++i) {
            ElfW(Rela) *curRelaPlt = curELFImgInfo.relaPlt + i;
            ssize_t idx = ELFW(R_SYM)(curRelaPlt->r_info);
            if (idx > maxID)
                maxID = idx;
        }
        return maxID;

    }

    void ExtFuncCallHookPtrace::recordPltCode(Hook::ExtSymInfo &curSymbol) {
        //Get the plt data of curSymbol
        //todo: .plt size is hard coded
        //Warning: this memory should be freed in ~PltCodeInfo
        void *pltEntry = pmParser.readProcMem(curSymbol.pltEntry, 16);

        auto &curPltCodeINfo = pltCodeInfoMap[curSymbol.fileId];

        curPltCodeINfo.pltCodeMap[curSymbol.funcId] = pltEntry;
        curPltCodeINfo.addrFuncMap[curSymbol.pltEntry] = curSymbol.funcId;
    }

    void ExtFuncCallHookPtrace::recordPltSecCode(Hook::ExtSymInfo &curSymbol) {
        //Get the plt data of curSymbol
        //todo: .plt.sec size is hard coded
        //Warning: this memory should be freed in ~PltCodeInfo
        void *pltSecEntry = pmParser.readProcMem(curSymbol.pltSecEntry, 16);

        auto &curPltCodeINfo = pltCodeInfoMap[curSymbol.fileId];

        curPltCodeINfo.pltSecCodeMap[curSymbol.funcId] = pltSecEntry;
        curPltCodeINfo.addrFuncMap[curSymbol.pltEntry] = curSymbol.funcId;
        curPltCodeINfo.addrFuncMap[curSymbol.pltSecEntry] = curSymbol.funcId;
    }

    void ExtFuncCallHookPtrace::instrumentPltSecCode(Hook::ExtSymInfo &curSymbol) {
        //todo: only apply to 64bit
        DBG_LOGS("Instrumented pltsec code for symbol:%p at:%p",curSymbol.symbolName.c_str(),curSymbol.pltSecEntry);
        const uint64_t &pltSecEntryCode = (uint64_t) pltCodeInfoMap.at(curSymbol.fileId).pltSecCodeMap.at(
                curSymbol.funcId);
        uint64_t pltSecEntryCodeWithTrap = (pltSecEntryCode & 0xFFFFFFFFFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, childPID, curSymbol.pltSecEntry, (void *) pltSecEntryCodeWithTrap);
    }

    void ExtFuncCallHookPtrace::debuggerLoop() {
        DBG_LOG("debugger started");
        /* The child can continue running now */
        ptrace(PTRACE_CONT, childPID, 0, 0);

        int wait_status;
        while (1) {
            DBG_LOGS("Wait for child", strsignal(WSTOPSIG(wait_status)));
            wait(&wait_status);
            if (WIFSTOPPED(wait_status)) {
                DBG_LOGS("Child got a signal: %s", strsignal(WSTOPSIG(wait_status)));
                parseSymbolInfo();
                preHookHandler();
            }
        }
        while (true) {
            if (WIFSTOPPED(wait_status)) {
                DBG_LOGS("Child got a signal: %s", strsignal(WSTOPSIG(wait_status)));
            } else {
                perror("wait Err");
            }
            if (WIFEXITED(wait_status)) {
                DBG_LOG("Child exited");
                break;
            }
        }


    }

    void ExtFuncCallHookPtrace::preHookHandler() {

    }

    void ExtFuncCallHookPtrace::parseSymbolInfo() {
        //Parse symbol info based on EIP
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, childPID, 0, &regs);
        //todo: we could also intercept a later instruction and parse function id from stack
        //todo: We could also map addr directly to both func and file id
        size_t fileID = pmParser.findExecNameByAddr(reinterpret_cast<void *>(regs.rip - 1));

        const auto &curELFImgInfo = elfImgInfoMap.at(fileID);


        DBG_LOGS("Child stopped at RIP = %p offset=%llu", regs.rip, regs.rip-1 - (uint64_t)curELFImgInfo.baseAddr);


        const auto &curPltCodeInfo = pltCodeInfoMap.at(fileID);
        auto curFuncID = curPltCodeInfo.addrFuncMap.at(reinterpret_cast<void *>(regs.rip-1));
        DBG_LOGS("%s in %s is called in %s", curELFImgInfo.idFuncMap.at(curFuncID).c_str(), "unknownLib",
                 curELFImgInfo.filePath.c_str());

    }


    ExtFuncCallHookPtrace::PltCodeInfo::~PltCodeInfo() {
        //Free plt code
        for (auto iter = pltCodeMap.begin(); iter != pltCodeMap.end(); ++iter) {
            if (iter->second != nullptr) {
                free(iter->second);
                iter->second = nullptr;
            }
        }
        //Free pltsec code
        for (auto iter = pltSecCodeMap.begin(); iter != pltSecCodeMap.end(); ++iter) {
            if (iter->second != nullptr) {
                free(iter->second);
                iter->second = nullptr;
            }
        }
    }
}


#endif
