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
#include <type/ELFSecInfo.h>

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

        Array<ELFImgInfo> elfImgInfoMap;//Mapping fileID to ELFImgInfo
        Array<ExtSymInfo> allExtSymbol;//All external symbols in ELF image
        Array<ssize_t> pltRelaIndexMap;//All external symbols in ELF image
        ssize_t hookedExtSymSize = 0;

        /**
         * Private constructor
         */
        explicit ExtFuncCallHook(PmParser &parser);

        inline bool isSymbolAddrResolved(ExtSymInfo &symInfo) {
//            //Check whether its value has 6 bytes offset as its plt entry start address
//            int64_t myPltStartAddr = (int64_t) symInfo.pltEntry;
//            int64_t curGotAddr = (int64_t) *symInfo.gotEntry;
//            assert(symInfo.pltEntry != nullptr);
//            int64_t offset = curGotAddr - myPltStartAddr;
//            return offset > 6 || offset < -6;
            return false;
        }

        static ExtFuncCallHook *getInst();

        static ExtFuncCallHook *instance;


    protected:
        inline bool shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type);


        inline bool
        parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltInfo, ELFSecInfo &pltSecInfo, ELFSecInfo &gotInfo,
                      uint8_t *baseAddr);

        bool
        installHook(std::string &fullPath, ELFParser &parser, ssize_t fileId, uint8_t *baseAddr, ELFSecInfo &pltSec,
                    ELFSecInfo &gotSec);

        bool makeGOTWritable(ELFSecInfo &gotSec, bool writable);

        uint32_t parsePltStubId(uint8_t *dest);
    };

}

extern "C" {


__pid_t fork(void);

}


#endif

#endif
