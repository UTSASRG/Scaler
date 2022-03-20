#ifndef SCALER_EXTFUNCCALLHOOK_H
#define SCALER_EXTFUNCCALLHOOK_H

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/MemoryTool.h>
#include <exceptions/ScalerException.h>
#include <util/datastructure/Hashmap.hh>
#include <util/datastructure/Vector.h>
#include <type/ExtSymInfo.h>
#include <type/ELFImgInfo.h>

namespace scaler {

    /**
    * Determine whether a symbol should be hooked
    */
    typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

    class ExtFuncCallHook {
    public:
        ExtFuncCallHook();

        ExtFuncCallHook(ExtFuncCallHook &) = delete;

        ExtFuncCallHook(ExtFuncCallHook &&) = delete;

        virtual bool install();

        virtual bool uninstall();

        virtual ~ExtFuncCallHook();

        PmParser pmParser;

        Vector<ELFImgInfo> elfImgInfoMap;//Mapping fileID to ELFImgInfo
        ssize_t numOfHookedELFImg = 0; //Some fileID corresponds to invalid elf img.
        Vector<SymID> hookedExtSymbol;//External symbol ids that has already been hooked
        Vector<ExtSymInfo> allExtSymbol;//All external symbols in ELF image

        /**
         * Private constructor
         */
        explicit ExtFuncCallHook(PmParser &parser);

        /**
         * This function fills address into binary instructions used to replace .plt and .ple.sec table
         * todo: This function is machine specific
         * todo: Add assemboly code to comments to make it more clear
         */
        bool fillDestAddr2PltRedirectorCode(void *funcAddr, std::vector<uint8_t> &retPltRedirCode);

        /**
        * Locate the address of required sections in memory
        */
        virtual bool locateRequiredSecAndSeg();

        inline bool isSymbolAddrResolved(ExtSymInfo &symInfo) {
            //Check whether its value has 6 bytes offset as its plt entry start address
            int64_t myPltStartAddr = (int64_t) symInfo.pltEntry;
            int64_t curGotAddr = (int64_t) *symInfo.gotEntry;
            assert(symInfo.pltEntry != nullptr);
            int64_t offset = curGotAddr - myPltStartAddr;
            return offset > 6 || offset < -6;
        }

        static ExtFuncCallHook *getInst();

        static ExtFuncCallHook *instance;

        void parsePltAddrInMem(ELFParser &elfParser, PMEntry &curPmEntry, void *&pltAddr, void *&pltSecAddr);

        bool installHook(std::string &dirName, std::string &fileName, ELFParser &parser, ssize_t prevFileId);
    protected:
        inline bool shouldHookThisSymbol(const char *funcName);
    };

}

extern "C" {


__pid_t fork(void);

}


#endif

#endif
