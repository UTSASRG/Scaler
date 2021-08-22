#ifndef SCALER_EXTFUNCCALLHOOKBRKPOINT_H
#define SCALER_EXTFUNCCALLHOOKBRKPOINT_H

#include "ExtFuncCallHook_Linux.hh"
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

        /**
        * PltCode information
        */
        class BrkPointInfo {
        public:
            BrkPointInfo() = default;

            BrkPointInfo(const BrkPointInfo &rho) = delete;

            void operator=(const BrkPointInfo &rho) = delete;


            std::map<void *, unsigned long *> brkpointCodeMap; //addr: original code

            std::map<void *, size_t> brkpointFuncMap; //Map plt/pltsec address to function id

            std::set<void *> brkpointPltAddr;

            ~BrkPointInfo();
        };


        PmParser_Linux pmParser;                            //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;

        static ExtFuncCallHookBrkpoint *instance; //Singleton

        /**
         * Private constructor
         */
        explicit ExtFuncCallHookBrkpoint();

        void parseRelaSymbol(ELFImgInfo &curELFImgInfo, size_t curFileID) override;


        size_t findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo);


        BrkPointInfo brkPointInfo;         //Mapping fileID to PltCodeInfo

        void recordOriCode(const size_t &funcID, void *addr, bool isPLT = false);

        void insertBrkpointAt(void *addr, int childTid);

        void debuggerLoop();

        void preHookHandler(size_t curFileID, size_t extSymbolId, void *callerAddr, void *brkpointLoc,
                             int childTid);

        void afterHookHandler(int childTid);

        bool parseSymbolInfo(size_t &curFileID, size_t &curFuncID, void *&callerAddr, void *&brkpointLoc,
                              int childTid);

        bool brkPointInstalledAt(void *addr);

        bool isBrkPointLocPlt(void *brkpointLoc);

        void skipBrkPoint(void *brkpointLoc, int childTid, bool continueAfterSkip = true);

        std::set<int> tracedTID;

        void brkpointEmitted(int childTid);

        void newThreadCreated(int childTid);

        void threadExited(int childTid);

        int waitForAllChild(int &waitStatus);

        void *findMainAddress();

        void waitBeforeMainExecution();
    };
}


#endif
