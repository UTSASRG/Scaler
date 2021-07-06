#ifndef SCALER_EXTFUNCCALLHOOK_LINUX_HH
#define SCALER_EXTFUNCCALLHOOK_LINUX_HH

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

        virtual void install(SYMBOL_FILTER filterCallB) = 0;


        virtual void uninstall() = 0;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &) = delete;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &&) = delete;

        virtual ~ExtFuncCallHook_Linux() override = 0;

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
        MemoryTool_Linux *memTool;

        /**
        * Locate the address of required sections in memory
        */
        void locateRequiredSecAndSeg();

        /**
         * Private constructor
         */
        ExtFuncCallHook_Linux();


    };

}


#endif

#endif
