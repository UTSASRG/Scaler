#include <link.h>
#include "hook.hh"
#include "../tool/ProcInfoParser.h"
#include "ExtFuncCallHook_Linux.hh"
#include <elf.h>
#include <vector>
#include <map>

#ifndef SCALER_EXTFUNCCALLHOOKPTRACE_H
#define SCALER_EXTFUNCCALLHOOKPTRACE_H

#endif //SCALER_EXTFUNCCALLHOOKPTRACE_H


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

        PmParser_Linux pmParser;                            //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;

        static ExtFuncCallHookPtrace *instance; //Singleton

        /**
         * Private constructor
         */
        ExtFuncCallHookPtrace(pid_t childPID);

        void locateRequiredSecAndSeg() override;

        pid_t childPID;


    };
}