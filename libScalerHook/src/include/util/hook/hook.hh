#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>
#include <cstdio>
#include <string>

namespace scaler {
    class Hook : Object {
    public:

        /**
         * Symbol information
         */
        class ExtSymInfo {
        public:
            std::string symbolName = "";     //The name of a symbol in symbol table
            void **gotEntry = nullptr;            //Pointer to a symbol's GOT entry. Use *gotEntry to get this symbol's real address.
            void *pltEntry = nullptr;            //Pointer to a symbol's PLT entry.
            void *pltSecEntry = nullptr;            //Pointer to a symbol's .plt.sec entry.
            void *addr = nullptr;                 //The address of a symbol. After a symbol is resolved, it's equal to *gotEntry;
            ssize_t fileId = -1;             //Store fileID for this symbol
            ssize_t extSymbolId = -1;             //The id with respect to where this symbol is called. Store this symbol's ID (it's also called symbolID) //todo: change this to symbolID for consistency
            ssize_t libraryFileID = -1;       //Store the libary file id that contains this
        };

        /**
         * Determine whether a symbol should be hooked
         */
        typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

        virtual void install(SYMBOL_FILTER filterCallB) = 0;

        virtual void uninstall() = 0;

    };

}


#endif