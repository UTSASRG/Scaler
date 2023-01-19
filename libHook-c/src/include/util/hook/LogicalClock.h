#ifndef SCALER_LOGICALCLOCK_H
#define SCALER_LOGICALCLOCK_H

#include <util/tool/Timer.h>
#include <util/hook/HookContext.h>
#include <immintrin.h>
#include <util/tool/SpinLock.h>

extern uint32_t threadNum;
extern uint64_t threadAttributionClock;
extern uint64_t wallclockSnapshot;

inline void initClockAttribution(HookContext *curContext) {
    threadNum = 1;
    threadAttributionClock = 0;
    curContext->startTImestamp = threadAttributionClock;
    wallclockSnapshot = getunixtimestampms();
}

inline void threadCreationRecord(HookContext *curContext) {
    uint64_t wallClockCurSnapshot = getunixtimestampms();

    uint32_t oriThreadNum = __atomic_fetch_add(&threadNum, 1, __ATOMIC_ACQ_REL);

    if (oriThreadNum == 0) {
        fatalError("Thread number is zero. Value incorrect. Check synchronization.");
    }

    threadAttributionClock += (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum;


    INFO_LOGS("StartTimestamp+=(%lu-%lu)/%d=%lu/%d=%lu", wallClockCurSnapshot, wallclockSnapshot,
              oriThreadNum, wallClockCurSnapshot - wallclockSnapshot, oriThreadNum,
              (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum);

    curContext->startTImestamp = threadAttributionClock;

    wallclockSnapshot=wallClockCurSnapshot;


    //Built-in Function: type __atomic_fetch_add (type *ptr, type val, int memorder)
//    curContext->startTImestamp = __atomic_add_fetch(&threadAttributionClock,
//                                                    threadAttributionClock +
//                                                    (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum,
//                                                    __ATOMIC_ACQ_REL);

//    //Built-in Function: void __atomic_load (type *ptr, type *ret, int memorder)
//    __atomic_load(&wallClockCurSnapshot, &wallclockSnapshot, __ATOMIC_ACQUIRE);
}

inline void threadTerminatedRecord(HookContext *curContext) {
    INFO_LOG("threadTerminatedRecord");

    uint64_t wallClockCurSnapshot = getunixtimestampms();

    uint32_t oriThreadNum = __atomic_fetch_sub(&threadNum, 1, __ATOMIC_ACQ_REL);

    if (oriThreadNum == 0) {
        fatalError("Thread number is zero. Value incorrect. Check synchronization.");
    }

    threadAttributionClock += (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum;
    curContext->endTImestamp = threadAttributionClock;

    INFO_LOGS("EndTimestamp+=(%lu-%lu)/%d=%lu/%d=%lu", wallClockCurSnapshot, wallclockSnapshot,
              oriThreadNum, wallClockCurSnapshot - wallclockSnapshot, oriThreadNum,
              (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum);


    wallclockSnapshot=wallClockCurSnapshot;


//    curContext->endTImestamp = __atomic_add_fetch(&threadAttributionClock,
//                                                  (wallClockCurSnapshot - wallclockSnapshot) / oriThreadNum,
//                                                  __ATOMIC_ACQ_REL);
//    __atomic_load(&wallClockCurSnapshot, &wallclockSnapshot, __ATOMIC_ACQUIRE);

}

#endif
