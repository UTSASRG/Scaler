//
// Created by st on 6/29/21.
//

#ifndef SCALER_TIME_H
#define SCALER_TIME_H

#include <inttypes.h>


inline uint64_t getunixtimestampms(){
    uint32_t lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((int64_t)hi << 32) | lo;
}


#endif //SCALER_TIME_H
