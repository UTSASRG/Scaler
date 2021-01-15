#ifndef SCALER_EXTFUNCCALLHOOKHANDLER_H
#define SCALER_EXTFUNCCALLHOOKHANDLER_H

#include <map>
#include <vector>

namespace scaler {
    class SecInfo {
    public:
        void *startAddr = nullptr;
        void *endAddr = nullptr;
        long long int itemSize = -1;
    };

    class SegInfo {
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
}

// Whether i'th external symbol has been resolved in a.so: realAddrResolved[id for a.so][i]
extern std::map<size_t, std::vector<bool>> realAddrResolved;
// The name of i'th hooked external function in a.so: hookedNames[id for a.so][i].name
extern std::map<size_t, std::vector<scaler::HookedExtSym>> hookedExtSymbols;


#endif //SCALER_EXTFUNCCALLHOOKHANDLER_H
