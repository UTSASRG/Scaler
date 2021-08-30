#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include "pthread.h"
#include <util/tool/Serializable.h>
#include <list>
#include <stdio.h>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <util/hook/ExtFuncCallHook_Linux.hh>
#include <bits/types/siginfo_t.h>

namespace scaler {

    typedef void (*callback_sa_sigaction)(int, siginfo_t *, void *);

    class Breakpoint {
    public:
        uint8_t *addr;
        // now it support one thread,
        // we can change it to thread mask later
        char oriCode[15];
        int instLen;

        size_t funcID; //funcID from scaler

        // callback function
        // callback_sa_sigaction func;

    };

}
#endif