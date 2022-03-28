
#ifndef SCALER_CONFIG_H
#define SCALER_CONFIG_H


#include <zconf.h>
#include <cstdint>

// Warining for memory leak. Static class

namespace scaler {
    static pthread_t mainthreadID = 0;
    static uint64_t libHookStartingAddr = 0;
    static uint64_t libHookEndingAddr = 0;
    static short maximumHierachy = 4;
}

#endif
