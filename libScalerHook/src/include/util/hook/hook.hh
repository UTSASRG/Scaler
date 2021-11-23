#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>
#include <cstdio>
#include <string>

namespace scaler {

    typedef ssize_t FileID;
    typedef ssize_t SymID;
    typedef ssize_t FuncID;


    class Hook : Object {
    public:

        /**
         * Determine whether a symbol should be hooked
         */
        typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

        virtual void install(SYMBOL_FILTER filterCallB) = 0;

        virtual void uninstall() = 0;

    };

}


#endif