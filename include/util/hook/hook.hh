#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>

namespace scaler {
    class Hook : Object {
    public:
        //A flag marking the entry of hook handler
        thread_local static bool SCALER_HOOK_IN_HOOK_HANDLER;

        virtual void install() = 0;

        virtual void uninstall() = 0;

    };
}


#endif
