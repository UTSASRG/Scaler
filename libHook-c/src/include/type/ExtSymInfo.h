#ifndef EXTSYMINFO_H
#define EXTSYMINFO_H

#include <string>

namespace scaler {

    typedef ssize_t FileID;
    typedef ssize_t SymID;
    typedef ssize_t FuncID;


    /**
    * Symbol information
    */
    struct ExtSymInfo {
        int64_t fileId = -1;//(8 bytes) Store fileID for this symbol
        int64_t symIdInFile = -1; //(8 bytes) todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
        uint8_t **gotEntryAddr = nullptr;//(8 bytes) The address of a symbol. If not resolved, == nullptr //This can be modified without lock because all threads will resolve to the same value. Replacing this memory with same value won't cause conflict.
        uint8_t *pltEntryAddr = nullptr; //(8 bytes)
        uint8_t *pltSecEntryAddr = nullptr; //(8 bytes)
        uint64_t pltStubId = 0; //(8 bytes)
        ssize_t initialGap = 0;//8 Bytes. Initial gap value
    };
}
#endif