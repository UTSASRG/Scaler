#ifndef SCALER_EXTFUNCCALLHOOK_HH
#define SCALER_EXTFUNCCALLHOOK_HH

#include <util/hook/hook.hh>

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/MemTool.h>

namespace scaler {


    class ExtFuncCallHook_Linux : public Hook {
    public:

        void install(SYMBOL_FILTER filterCallB) override;


        void uninstall() override;

        /**
         * Singeleton
         */
        static ExtFuncCallHook_Linux *getInst();

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &) = delete;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &&) = delete;

        ~ExtFuncCallHook_Linux() override;

    protected:


        /**
         * ELF image (ELF file in memory) information.
         */
        class ELFImgInfo {
        public:
            std::string filePath = "";                           //The absolute path of an ELF image (It's consistent with /proc/{pid}/maps)
            void *pltStartAddr = nullptr;                   //The starting address of the PLT table
            void *pltEndAddr = nullptr;                     //The ending address of the PLT table
            void *pltSecStartAddr = nullptr;                //The starting address of the PLT.SEC table
            void *pltSecEndAddr = nullptr;                  //The ending address of the PLT.SEC table
            ElfW(Dyn) *_DYNAMICAddr = nullptr;              //The staring address of _DYNAMIC

            std::vector<bool> realAddrResolved;             //Whether function with id i has been resolved.
            bool *realAddrResolvedC = nullptr;
            size_t realAddrResolvedCSize = 0;


            uint8_t *pseudoPlt = nullptr;                   //A pointer to pseudoPlt table

            std::map<size_t, ExtSymInfo> hookedExtSymbol;   //External symbols that has already been hooked

            ExtSymInfo *hookedExtSymbolC = nullptr;
            size_t hookedExtSymbolCSize = 0;

            std::map<size_t, ExtSymInfo> allExtSymbol;      //All external symbols in ELF image
            std::map<std::string, size_t> funcIdMap;        //Mapping function name to it's id
            std::map<size_t, std::string> idFuncMap;        //Mapping function id to it's name

            //todo: Check const for all variables
            ElfW(Rela) *relaPlt = nullptr;                            //The first .plt.rela entry in ELF iamge
            ElfW(Xword) relaPltCnt = 0;                         //The number of entries in relaPlt
            const ElfW(Sym) *dynSymTable = nullptr;                   //The first .dynamic entry in ELF image
            const char *dynStrTable = nullptr;                        //The starting position of dynamic symbol name
            size_t dynStrSize = 0;                              //The size of dynamic string table

            uint8_t *baseAddr = nullptr;                              //The loading address of current elf image

            ~ELFImgInfo();

            ELFImgInfo();

            ELFImgInfo(const ELFImgInfo &rho);

            void operator=(const ELFImgInfo &rho);


        };


        PmParserC_Linux pmParser;                            //A parser to /proc/self/maps
        std::map<size_t, ELFImgInfo> elfImgInfoMap;         //Mapping fileID to ELFImgInfo

        ELFImgInfo *elfImgInfoMapC = nullptr;
        size_t elfImgInfoMapCSize = 0;

        MemoryTool_Linux *memTool;

        static ExtFuncCallHook_Linux *instance; //Singleton

        /**
        * Locate the address of required sections in memory
        */
        void locateRequiredSecAndSeg();

        /**
         * Private constructor
         */
        ExtFuncCallHook_Linux();


        /**
         * This function fills address into binary instructions used to replace .plt and .ple.sec table
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

        /**
         * A handler written in C. It calls custom handler and calculates actual function address
         * In the new code, .plt and .plt.sec uses the same handler. Since we currently don't calculate
         * based on the first address.
         * @param callerFuncAddr The next caller
         * @return Original function pointer
         */
        static void *cPreHookHanlderLinuxSec(size_t fileId, size_t funcId, void *callerAddr);

        //static __attribute__((optimize("O0"))) void *cPreHookHanlderLinux(void *pltEntryAddr, void *callerAddr);

        static void *cAfterHookHanlderLinux();


        uint8_t *autoAddBaseAddr(uint8_t *addr, size_t fileiD, Elf64_Addr dPtr);

        void *writeAndCompileHookHanlder(std::vector<ExtSymInfo> &symbolToHook);

        void *writeAndCompilePseudoPlt(std::vector<ExtSymInfo> &symbolToHook);

    };

}

#define DECL_PREHOOK(suffix) void *cPreHookHanlderLinux##suffix(int index, void *callerFuncAddr);


extern "C" {
DECL_PREHOOK()

DECL_PREHOOK(Sec)
}

#endif

#endif
