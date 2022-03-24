#ifndef EXTSYMINFO_H
#define EXTSYMINFO_H

#include <string>

namespace scaler {

    typedef short FileID;
    typedef short SymID;
    typedef short FuncID;


    /**
    * Symbol information
    */
    struct ExtSymInfo {
        FileID fileId = -1;//Store fileID for this symbol
        SymID symIdInFile = -1; //todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
        uint8_t *resolvedAddr = nullptr;//The address of a symbol. If not resolved, == nullptr //This can be modified without lock because all threads will resolve to the same value. Replacing this memory with same value won't cause conflict.
        uint64_t strTableOffset;
        ssize_t hookedId = 0;
    };
}
#endif