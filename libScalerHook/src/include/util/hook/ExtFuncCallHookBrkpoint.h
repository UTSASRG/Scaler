#ifndef SCALER_EXTFUNCCALLHOOKBRKPOINT_H
#define SCALER_EXTFUNCCALLHOOKBRKPOINT_H

#include "ExtFuncCallHook_Linux.hh"
#include <type/breakpoint.h>
#include <ucontext.h>
#include <set>

namespace scaler {

    class ExtFuncCallHookBrkpoint : public ExtFuncCallHook_Linux {
    public:

        void install(SYMBOL_FILTER filterCallB) override;


        void uninstall() override;

        /**
         * Singeleton
         */
        static ExtFuncCallHookBrkpoint *getInst();

        ExtFuncCallHookBrkpoint(ExtFuncCallHookBrkpoint &) = delete;

        ExtFuncCallHookBrkpoint(ExtFuncCallHookBrkpoint &&) = delete;

        ~ExtFuncCallHookBrkpoint() override;

        void
        parseFuncInfo(size_t callerFileID, int64_t fileIDInCaller, void *&funcAddr, int64_t &libraryFileID) override;


    protected:

        PmParser_Linux pmParser;                            //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;

        static ExtFuncCallHookBrkpoint *instance; //Singleton

        /**
         * Private constructor
         */
        explicit ExtFuncCallHookBrkpoint();

        void parseRelaSymbol(ELFImgInfo &curELFImgInfo, size_t curFileID) override;


        size_t findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo);


        std::map<void *, Breakpoint> brkPointInfo;         //Mapping fileID to PltCodeInfo

        std::set<void *> brkpointPltAddr;


        void recordBrkpointInfo(const size_t &funcID, void *addr, bool isPLT = false);

        void insertBrkpointAt(Breakpoint &bp);

        void preHookHandler(size_t curFileID, size_t extSymbolId, void *callerAddr, void *brkpointLoc,
                            pthread_t childTid);

        void afterHookHandler(pthread_t childTid);

        static bool parseSymbolInfo(void *&callerAddr,ucontext_t *context);

        bool brkPointInstalledAt(void *addr);

        bool isBrkPointLocPlt(void *brkpointLoc);

        void skipBrkPoint(Breakpoint& bp,ucontext_t* context);

        std::set<int> tracedTID;

        static void brkpointEmitted(int signum, siginfo_t *siginfo, void *context);


        void newThreadCreated(int childTid);

        int waitForAllChild(int &waitStatus);

        void *findMainAddress();

        void waitBeforeMainExecution();

        void installSigIntHandler();
    };
}


#endif
