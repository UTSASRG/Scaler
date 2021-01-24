#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>
#include <cstdio>

namespace scaler {
    class Hook : Object {
    public:

        virtual void install() = 0;

        virtual void uninstall() = 0;

    };

}


#endif