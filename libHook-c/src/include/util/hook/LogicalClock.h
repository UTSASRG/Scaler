#ifndef SCALER_LOGICALCLOCK_H
#define SCALER_LOGICALCLOCK_H

#include "util/tool/Timer.h"

extern uint8_t threadAttributionRDLock = 0; //A lock, used to ensure simulataneous writing of threadAttributionClock and wallclockSnapshot
extern uint8_t threadAttributionRWLock = 0; //A lock, used to ensure simulataneous writing of threadAttributionClock and wallclockSnapshot
extern uint32_t threadNum;
extern uint64_t threadAttributionClock;
extern uint64_t wallclockSnapshot;


inline void acquireLogicalClockLockRW() {
    while (__atomic_test_and_set(&threadAttributionLock, __ATOMIC_ACQ_REL));
}

inline void releaseLogicalClockLock() {
    __atomic_clear(&threadAttributionLock, __ATOMIC_RELEASE);
}

inline void applicationStartRecord() {
    wallclockSnapshot = getunixtimestampms();
    threadNum = 1;
    threadAttributionClock = 0;
}

inline void threadCreatedRecord() {
    acquireLogicalClockLock();

    uint64_t wallClockCurSnapshot = getunixtimestampms();
    threadAttributionClock += (wallClockCurSnapshot - wallclockSnapshot) / threadNum;
    threadNum += 1;
    wallclockSnapshot = wallClockCurSnapshot;
    releaseLogicalClockLock();
}

inline void threadTerminatedRecord() {
    acquireLogicalClockLock();

    uint64_t wallClockCurSnapshot = getunixtimestampms();
    threadAttributionClock += (wallClockCurSnapshot - wallclockSnapshot) / threadNum;
    threadNum -= 1;
    wallclockSnapshot = wallClockCurSnapshot;

    releaseLogicalClockLock();
}

inline void functionFinishRecord(){

}

#endif
