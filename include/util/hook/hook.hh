#ifndef SCALER_HOOK_HH
#define SCALER_HOOK_HH

#include <type/Object.hpp>

class Hook : Object {
    virtual void install() = 0;
};



#endif
