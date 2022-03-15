#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>
#include <cstdio>
#include <string>

namespace scaler {

    typedef short FileID;
    typedef short SymID;
    typedef short FuncID;


    class Hook : Object {
    public:

        /**
         * Determine whether a symbol should be hooked
         */
        typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

        virtual bool install(SYMBOL_FILTER filterCallB) = 0;

        virtual bool uninstall() = 0;

    };

}


#endif