#ifndef SCALER_EXTFUNCCALLHOOKASM_HH
#define SCALER_EXTFUNCCALLHOOKASM_HH

#include <util/hook/hook.hh>

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/MemTool.h>
#include "ExtFuncCallHook_Linux.hh"
#include <util/datastructure/Stack.h>
#include <util/tool/AssemblyTool.h>

namespace scaler {

    class ExtFuncCallHookAsm : public ExtFuncCallHook_Linux {
    public:

        void install(SYMBOL_FILTER filterCallB) override;


        void uninstall() override;

        /**
         * Singeleton
         */
        static ExtFuncCallHookAsm *getInst();

        ExtFuncCallHookAsm(ExtFuncCallHookAsm &) = delete;

        ExtFuncCallHookAsm(ExtFuncCallHookAsm &&) = delete;

        ~ExtFuncCallHookAsm() override;

        //void saveCommonFuncID();

        void
        parseFuncInfo(FileID callerFileID, FuncID symbolIDInCaller, void *&funcAddr, FileID &libraryFileID) override;

        PmParserC_Linux pmParser;

        int64_t appStartTimestamp;
    protected:

        //A parser to /proc/self/maps
        MemoryTool_Linux *memTool;
        static ExtFuncCallHookAsm *instance; //Singleton

        /**
         * Private constructor
         */
        ExtFuncCallHookAsm();


        /**
         * This function fills address into binary instructions used to replace .plt and .ple.sec table
         * todo: This function is machine specific
         * todo: Add assemboly code to comments to make it more clear
         */
        std::vector<uint8_t> fillDestAddr2PltRedirectorCode(void *funcAddr);

        /**
        * This function fills address into binary file  (Binary code used to store in Heap as Pseudo Plt table)
        * todo: This function is machine specific
        * todo: Add assemboly code to comments to make it more clear
        */
//        std::vector<uint8_t> fillDestAddr2PseudoPltCode(SymID extSymbolId, void *funcAddr);

        void *writeAndCompileRedzoneJumper();

        void *writeAndCompilePseudoPlt();

        void parsePltEntryAddress();

    };

}


extern "C" {

/**
 * A handler written in C. It calls custom handler and calculates actual function address
 * In the new code, .plt and .plt.sec uses the same handler. Since we currently don't calculate
 * based on the first address.
 * @param callerFuncAddr The next caller
 * @param oriRBPLoc The rsp location before saving all registers
 * @return Original function pointer
 */
__attribute__((used)) static void *
cPreHookHandlerLinux(scaler::SymID extSymbolId, void *callerAddr);


__attribute__((used)) static void *cAfterHookHandlerLinux();

}

extern "C" {
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
//void pthread_exit(void *__retval);
}

#endif

#endif

