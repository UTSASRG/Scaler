#ifndef SCALER_LOGICALCLOCK_H
#define SCALER_LOGICALCLOCK_H

#include <util/tool/Timer.h>
#include <util/hook/HookContext.h>

extern uint32_t threadNum;
extern uint64_t threadAttributionClock;
extern uint64_t wallclockSnapshot;
extern __thread bool curThreadTimestmapSaved; //Make sure time is only saved once

inline void initClockAttribution(HookContext *curContext) {
    threadNum = 1;
    threadAttributionClock = 0;
    curContext->startTImestamp = threadAttributionClock;
    wallclockSnapshot = getunixtimestampms();
}

inline void threadCreationRecord(HookContext *curContext) {
    curContext->startTImestamp = threadAttributionClock;

    uint64_t wallClockCurSnapshot = getunixtimestampms();

    //Built-in Function: type __atomic_fetch_add (type *ptr, type val, int memorder)
    __atomic_fetch_add(&threadAttributionClock,
                       threadAttributionClock + (wallClockCurSnapshot - wallclockSnapshot) / threadNum,
                       __ATOMIC_ACQ_REL);
    __atomic_add_fetch(&threadNum, 1, __ATOMIC_ACQ_REL);

    //Built-in Function: void __atomic_load (type *ptr, type *ret, int memorder)
    __atomic_load(&wallClockCurSnapshot, &wallclockSnapshot, __ATOMIC_ACQUIRE);
}

inline void threadTerminatedRecord(HookContext *curContext) {
    if (curThreadTimestmapSaved) {
        return;
    }
    curThreadTimestmapSaved = true;

    curContext->endTImestamp = threadAttributionClock;

    uint64_t wallClockCurSnapshot = getunixtimestampms();

    __atomic_fetch_add(&threadAttributionClock, (wallClockCurSnapshot - wallclockSnapshot) / threadNum,
                       __ATOMIC_ACQ_REL);
    __atomic_sub_fetch(&threadNum, 1, __ATOMIC_ACQ_REL);

    __atomic_load(&wallClockCurSnapshot, &wallclockSnapshot, __ATOMIC_ACQUIRE);

}

#endif
