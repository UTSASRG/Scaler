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

        /**
         * Locate the start position of sections in memory and store then in fileSecMap
         */
        void locSectionInMem();
    private:
        void *startPlt = nullptr;
        void *startGot = nullptr;
        void *startPltSec = nullptr;

        //Get the pointer to the first section: sectionAddrMap[executable path][section name]
        std::map<std::string, std::map<std::string, SecInfo>> fileSecMap;
        std::map<std::string, std::vector<std::string>> filePltNameMap;

        /**
         * Search the starting address of a single loaded section in memory
         */
        void *searchSecLoadingAddr(const std::string &secName, ELFParser &elfParser,
                                   const std::vector<PMEntry> &segments);

        void loadPltNames();

    };
}

#endif
