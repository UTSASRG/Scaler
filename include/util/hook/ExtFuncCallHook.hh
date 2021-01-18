#ifndef SCALER_EXTFUNCCALLHOOK_HH
#define SCALER_EXTFUNCCALLHOOK_HH

#include <util/hook/hook.hh>
#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/PMParser.h>

thread_local static bool SCALER_HOOK_IN_HOOK_HANDLER = false;

namespace scaler {
    //todo: Refactor into structures similar to ELF format but more convenient to use. Rather than fully customized structure
    class SecInfo {
    public:
        void *startAddr = nullptr;
        void *endAddr = nullptr;
        unsigned long long int itemSize = -1;
        unsigned long long int secTotalSize = -1;
    };


    class SegInfo {
    public:
        void *startAddr = nullptr;
    };

    class LoadingInfo {
    public:
        void *startAddr = nullptr;
        void *endAddr = nullptr;
        std::string fileName;

    };

    class HookedExtSym {
    public:
        std::string name;
        size_t id;
    };

    //A flag marking the entry of hook handler
    thread_local static bool SCALER_HOOK_IN_HOOK_HANDLER = false;

    enum SEC_NAME {
        PLT,
        PLT_SEC,
        GOT,
        DYNAMIC
    };

    class ExtFuncCallHook : public Hook {
    public:


        static ExtFuncCallHook *instance; //Singeleton

        // The pointer to .plt in a.so: sectionAddrMap[id for a.so][PLT] (Used to store interested sections) //todo: Better naming?
        std::map<size_t, std::map<SEC_NAME, SecInfo>> fileSecInfoMap;
        //The pointer to _DYNAMIC segment in a.so: sectionAddrMap[id for a.so][PT_DYNAMIC][0]

        std::map<size_t, std::map<ElfW(Word), SegInfo>> fileSegInfoMap;


        // The i'th external symbol's name in a.so: sectionAddrMap[id for a.so][i]
        std::map<size_t, std::map<std::string, size_t>> fileExtFuncNameMap;
        // The id of a.so : fileNameIDMap[full path for a.so]
        std::map<std::string, size_t> fileIDMap;

        // Used to find which fileID  floor(i/2) the corresponding fileID of pointer addrFileMap[i]
        // This array should be sorted for fast lookup (Reflect /self/proc/maps content)
        std::vector<LoadingInfo> fileLoadMap;
        // The first GOT entry of a.so : fileGotMap[id for a.so][0]
        std::map<size_t, std::vector<void *>> fileGotMap;


        //Used by hook handler
        // Whether i'th external symbol has been resolved in a.so: realAddrResolved[id for a.so][i]
        std::map<size_t, std::vector<bool>> realAddrResolved;
        // The name of i'th hooked external function in a.so: hookedNames[id for a.so][i].name
        std::map<size_t, std::vector<scaler::HookedExtSym>> hookedExtSymbols;
        // The adddress of i'th hooked external function in a.so: hookedNames[id for a.so][i]
        std::map<size_t, std::vector<void *>> hookedAddrs;
        std::map<size_t, std::vector<std::string>> hookedFuncNames;
        uint8_t *pseudoPlt = nullptr;


        ExtFuncCallHook(ExtFuncCallHook &) = delete;

        ExtFuncCallHook(ExtFuncCallHook &&) = delete;

        static ExtFuncCallHook *getInst();

        void install() override;

        /**
         * Return addr is located in which
         * @param addr
         */
        size_t findExecNameByAddr(void *addr);

        void *getFuncAddrFromGOTByName(size_t fileId, std::string name);

    private:
        ExtFuncCallHook();


        // An array to the copy of orignal PLT table
        uint8_t *oriPltBin;

        /**
         * Locate the start position of sections in memory and store then in fileSecMap
         */
        void locSecAndSegInMem();

        /**
         * Save got into fileGotMap
         * Called by locSecAndSegInMem
         */
        void recordGOT(SecInfo &info, size_t fileId);

        /**
         * Search the start, end address of a loaded section in memory
         */
        void *searchSecLoadingAddr(std::string secName, ELFParser &elfParser,
                                   const std::vector<PMEntry> &segments);


        /**
         * Search the start, end address of a loaded segment in memory
         */
        void *searchSegLoadingAddr(ELFParser &elfParser, ElfW(Word) segType, const std::vector<PMEntry> &segments);

        void adjustMemPermission(void *startPtr, void *endPtr, int prem);


        /**
         * Record segment address -> fileID info into segAddrFileMap
         * @param pmParser
         */
        void recordFileSegMap(PMParser &pmParser);


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


    };


}

static scaler::ExtFuncCallHook *__extFuncCallHookPtr;

#define DECL_PREHOOK(suffix) \
void *cPreHookHanlder##suffix(int index, void *callerFuncAddr);


extern "C" {
DECL_PREHOOK()

DECL_PREHOOK(Sec)
}


#endif
