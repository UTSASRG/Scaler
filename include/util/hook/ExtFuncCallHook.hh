#ifndef SCALER_EXTFUNCCALLHOOK_HH
#define SCALER_EXTFUNCCALLHOOK_HH

#include <util/hook/hook.hh>
#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/PMParser.h>

namespace scaler {
    class SecInfo {
    public:
        void *startAddr = nullptr;
        void *endAddr = nullptr;
        long long int itemSize = -1;
    };

    class ExtFuncCallHook : public Hook {
    public:
        void install() override;


    private:
        // The pointer to .plt in a.so: sectionAddrMap[id for a.so][".plt"]
        std::map<size_t, std::map<std::string, SecInfo>> fileSecMap;
        // The i'th external symbol's name in a.so: sectionAddrMap[id for a.so][i]
        std::map<size_t, std::vector<std::string>> filePltNameMap;
        // The id of a.so : symbolNames[full path for a.so]
        std::map<std::string, size_t> symbolNames;
        std::vector<void *> addrFileMap;

        // An array to the copy of orignal PLT table
        uint8_t *oriPltBin;

        /**
         * Locate the start position of sections in memory and store then in fileSecMap
         */
        void locSectionInMem();
        /**
         * Search the starting address of a single loaded section in memory
         */
        void *searchSecLoadingAddr(std::string secName, ELFParser &elfParser,
                                   const std::vector<PMEntry> &segments);

        void adjustMemPermission(void *startPtr, void *endPtr, int prem);
    };
}

#endif
