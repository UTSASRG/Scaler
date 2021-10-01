#include <link.h>
#include "hook.hh"
#include "../tool/ProcInfoParser.h"
#include "ExtFuncCallHook_Linux.hh"
#include <elf.h>
#include <vector>
#include <map>
#include <set>
#include <sys/user.h>

#ifndef SCALER_EXTFUNCCALLHOOKPTRACE_H
#define SCALER_EXTFUNCCALLHOOKPTRACE_H

namespace scaler {

    /**
     * A special version of ExtFuncCallHook for ptrace.
     * The difference is locateRequiredSecAndSeg() cannot read another process automatically.
     */
    class ExtFuncCallHookPtrace : public ExtFuncCallHook_Linux {
    public:

        void install(SYMBOL_FILTER filterCallB) override;


        void uninstall() override;

        /**
         * Singeleton
         */
        static ExtFuncCallHookPtrace *getInst(pid_t tracedFilePath);

        ExtFuncCallHookPtrace(ExtFuncCallHookPtrace &) = delete;

        ExtFuncCallHookPtrace(ExtFuncCallHookPtrace &&) = delete;

        ~ExtFuncCallHookPtrace() override;

        void
        parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr, FileID &libraryFileID) override;


    protected:

        /**
        * PltCode information
        */
        class BrkPointInfo {
        public:
            BrkPointInfo() = default;

            BrkPointInfo(const BrkPointInfo &rho) = delete;

            void operator=(const BrkPointInfo &rho) = delete;


            std::map<void *, unsigned long *> brkpointCodeMap; //addr: original code

            std::map<void *, FuncID> brkpointFuncMap; //Map plt/pltsec address to function id

            std::set<void *> brkpointPltAddr;

            ~BrkPointInfo();
        };


        PmParser_Linux pmParser;                            //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;

        static ExtFuncCallHookPtrace *instance; //Singleton

        /**
         * Private constructor
         */
        explicit ExtFuncCallHookPtrace(pid_t childPID);

        void parseRelaSymbol(ELFImgInfo &curELFImgInfo, FileID curFileID) override;


        pid_t childMainThreadTID;


        ssize_t findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo);


        BrkPointInfo brkPointInfo;         //Mapping fileID to PltCodeInfo

        void recordOriCode(const FuncID &funcID, void *addr, bool isPLT = false);

        void insertBrkpointAt(void *addr, int childTid);

        void debuggerLoop();

        void preHookHandler(FileID curFileID, SymID extSymbolId, void *callerAddr, void *brkpointLoc,
                            user_regs_struct &regs, int childTid);

        void afterHookHandler(int childTid);

        bool parseSymbolInfo(FileID &curFileID, FuncID &curFuncID, void *&callerAddr, void *&brkpointLoc,
                             user_regs_struct &regs, int childTid);

        bool brkPointInstalledAt(void *addr);

        bool isBrkPointLocPlt(void *brkpointLoc);

        void skipBrkPoint(void *brkpointLoc, user_regs_struct &regs, int childTid, bool continueAfterSkip = true);

        std::set<int> tracedTID;

        void brkpointEmitted(int childTid);

        void newThreadCreated(int childTid);

        void threadExited(int childTid);

        int waitForAllChild(int &waitStatus);

        void *findMainAddress();

        void waitBeforeMainExecution();
    };
}

#endif //SCALER_EXTFUNCCALLHOOKPTRACE_H
