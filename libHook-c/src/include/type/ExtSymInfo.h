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
    class ExtSymInfo {
    public:
        std::string symbolName = "";     //The name of a symbol in symbol table
        void **gotEntry = nullptr;            //Pointer to a symbol's GOT entry. Use *gotEntry to get this symbol's real address.
        void *pltEntry = nullptr;            //Pointer to a symbol's PLT entry.
        void *pltSecEntry = nullptr;            //Pointer to a symbol's .plt.sec entry.
        void *addr = nullptr;                 //The address of a symbol. After a symbol is resolved, it's equal to *gotEntry; If the symbol is not resolve, it equals nullptr
        FileID fileId = -1;             //Store fileID for this symbol
        SymID symIdInFile = -1;             //The id with respect to where this symbol is called. Store this symbol's ID (it's also called symbolID) //todo: change this to symbolID for consistency
        SymID scalerSymbolId = -1; //todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
        FileID libraryFileScalerID = -1;       //Store the libary file id that contains this
        void *pseudoPltEntry = nullptr;                   //A pointer to pseudoPltEntry
        int type = -1;
        int bind = -1;
        int hookedId = -1; //Indicating whether this symbol has been hooked or not. If it is hooked, hookedId is the index in scaler::ExtFuncCallHookAsm::hookedExtSymbol
        void *oriPltSecCode = nullptr;
        void *oriPltCode = nullptr;

        bool operator==(const ExtSymInfo &rho) const;

        inline bool isHooked() const {
            return hookedId != -1;
        }
    };
}
#endif