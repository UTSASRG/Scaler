#ifndef SCALER_EXTFUNCCALLHOOK_HH
#define SCALER_EXTFUNCCALLHOOK_HH

#include <util/hook/hook.hh>

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>

namespace scaler {


    class ExtFuncCallHook_Linux : public Hook {
    public:
        class ExtSym {
        public:
            std::string symbolName;
            void **gotTableAddr;
            void *funcAddr;
        };

        class ELFImgInfo {
        public:
            std::string fileName;
            void *pltStartAddr = nullptr;
            void *pltEndAddr = nullptr;
            void *pltSecStartAddr = nullptr;
            void *pltSecEndAddr = nullptr;
            ElfW(Dyn) *_DYNAMICAddr = nullptr;

            std::vector<bool> realAddrResolved;

            uint8_t *pseudoPlt = nullptr;

            std::map<size_t, ExtSym> hookedExtSymbol;
            std::map<size_t, ExtSym> allExtSymbol;
            std::map<std::string, size_t> funcIdMap;
            std::map<size_t,std::string> idFuncMap;



            //todo: Check const for all variables
            ElfW(Rela) *relaPlt;
            ElfW(Xword) relaPltCnt;
            const ElfW(Sym) *dynSymTable;
            const char *dynStrTable;
            size_t dynStrSize;

            uint8_t *baseAddr;
        };

        void install() override;

        void uninstall() override;

        void updateGotAddr(ELFImgInfo& curELFImgInfo,size_t funcID);

        static ExtFuncCallHook_Linux *getInst();

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &) = delete;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &&) = delete;


        PmParser_Linux pmParser;
        std::map<size_t, ELFImgInfo> elfImgInfoMap;
    protected:


        static ExtFuncCallHook_Linux *instance; //Singleton

        //Map file id with it's own ELFImgInfo struct

        /**
         * Prohibit initialization
         */
        ExtFuncCallHook_Linux();


        /**
         * Locate the address of required sections in memory
         */
        void locateRequiredSecAndSeg();

        /**
         * Search the start, end address of a loaded section in memory
         * @param secPtrInFile: The section pointer in ELF file
         * @param firstEntrySize: # of bytes to search. The size of the first entry would be ideal,
         * because memory alignment typically happens after one entry
         * @param segments Specify wich segment to search
         */
        void *searchBinInMemory(void *secPtrInFile, size_t firstEntrySize,
                                const std::vector<PMEntry_Linux> &segments);


        /**
         * This function fills address into binary instruction array (Binary code used to replace .plt and .ple.sec)
         * todo: This function is machine specific
         * todo: Add assemboly code to comments to make it more clear
         */
        std::vector<uint8_t> fillDestAddr2HookCode(void *funcAddr);

        /**
        * This function fills address into binary file  (Binary code used to store in Heap as Pseudo Plt table)
        * todo: This function is machine specific
        * todo: Add assemboly code to comments to make it more clear
        */
        std::vector<uint8_t> fillDestAddr2PseudoPltCode(size_t funcId, void *funcAddr);

        /**
        * Find entry in Dyn, compare it's tag with parameter "tag"
        * @return Matched Dyn entry
        */
        ElfW(Dyn) *findDynEntryByTag(ElfW(Dyn) *dyn, ElfW(Sxword) tag);

        friend void *cPreHookHanlderLinuxSec(int index, void *callerFuncAddr);


    };

}

static scaler::ExtFuncCallHook_Linux *__extFuncCallHookPtr;

#define DECL_PREHOOK(suffix) void *cPreHookHanlderLinux##suffix(int index, void *callerFuncAddr);


extern "C" {
DECL_PREHOOK()

DECL_PREHOOK(Sec)
}

#endif

#endif
