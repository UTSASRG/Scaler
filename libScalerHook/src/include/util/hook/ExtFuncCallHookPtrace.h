#include <link.h>
#include "hook.hh"
#include "../tool/ProcInfoParser.h"
#include "ExtFuncCallHook_Linux.hh"
#include <elf.h>
#include <vector>
#include <map>

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

    protected:

        /**
        * PltCode information
        */
        class PltCodeInfo {
        public:
            PltCodeInfo() = default;

            PltCodeInfo(const PltCodeInfo &rho) = delete;

            void operator=(const PltCodeInfo &rho) = delete;

            std::map<size_t, void *> pltCodeMap;

            std::map<size_t, void *> pltSecCodeMap;


            ~PltCodeInfo();
        };


        PmParser_Linux pmParser;                            //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;

        static ExtFuncCallHookPtrace *instance; //Singleton

        /**
         * Private constructor
         */
        explicit ExtFuncCallHookPtrace(pid_t childPID);

        void parseRelaSymbol(ELFImgInfo &curELFImgInfo, size_t curFileID) override;


        pid_t childPID;


        size_t findDynSymTblSize(ExtFuncCallHook_Linux::ELFImgInfo &curELFImgInfo);


        std::map<size_t, PltCodeInfo> pltCodeInfoMap;         //Mapping fileID to PltCodeInfo

        void recordPltCode(ExtSymInfo &curSymbol);

        void recordPltSecCode(ExtSymInfo &curSymbol);

        void instrumentPltSecCode(ExtSymInfo &curSymbol);
    };
}

#endif //SCALER_EXTFUNCCALLHOOKPTRACE_H
