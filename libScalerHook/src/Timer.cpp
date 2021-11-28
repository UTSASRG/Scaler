//
// Created by st on 6/29/21.
//

#include <util/tool/Timer.h>
#include <time.h>
#include <math.h>

uint64_t getunixtimestampms() {
    uint32_t lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}