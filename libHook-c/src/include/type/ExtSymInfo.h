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
        uint8_t *gotAddr = nullptr;//The address of a symbol. After a symbol is resolved, it's equal to *gotEntry; If the symbol is not resolve, it equals nullptr
        FileID fileId = -1;//Store fileID for this symbol
        SymID symIdInFile = -1; //todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
    };
}
#endif