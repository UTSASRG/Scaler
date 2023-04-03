#ifndef SCALER_LOGICALCLOCK_H
#define SCALER_LOGICALCLOCK_H

#include <util/tool/Timer.h>
#include <util/tool/AtomicSpinLock.h>
#include <atomic>

extern uint64_t logicalClock; //The logical clock
extern uint32_t threadNum; //The current number of threads
extern std::atomic<uint64_t> wallclockSnapshot; //Used as update timestamp
extern AtomicSpinLock threadUpdateLock;

/**
 * Initialize logical clock, should be invoked before main thread starts execution
 * @param cachedWallClockSnapshot Reference to thread local timestamp used to determine modification or not.
 * @return Current wall clock snapshot
 */
inline uint64_t initLogicalClock(uint64_t &cachedWallClockSnapshot) {
    threadUpdateLock.lock();
    threadNum = 1;
    logicalClock = 0; //Logical clock should be 0 at first
    uint64_t curWallclockTimestamp = getunixtimestampms();
    wallclockSnapshot.store(curWallclockTimestamp, std::memory_order_release); //Publish thread number and logical clock
    cachedWallClockSnapshot = curWallclockTimestamp;
//    INFO_LOGS("Logical clock value initialized to = %lu", logicalClock);
    threadUpdateLock.unlock();
    return wallclockSnapshot;
}

/**
 * Update logical clock when thread creates/finished
 * Should only be invoked by API functions
 * @param threadNumChange +1 means a thread is being created, -1 means a thread is being destroyed
 * @param cachedWallClockSnapshot
 * @return
 */
inline uint64_t updateLogicalClockAndThreadNum(int8_t threadNumChange, uint64_t &cachedWallClockSnapshot) {
    threadUpdateLock.lock(); //Make sure that the following non-atomic operation will not cause data race.
    uint64_t curWallClockCurSnapshot = getunixtimestampms(); //Record current time
    uint64_t prevWallClockSnapshot = wallclockSnapshot.load(std::memory_order_acquire);
    logicalClock += (curWallClockCurSnapshot - prevWallClockSnapshot) / threadNum; //Update logical clock
//    INFO_LOGS("logicalClock += (%lu - %lu) / %u= %lu", curWallClockCurSnapshot, prevWallClockSnapshot, threadNum,
//              (curWallClockCurSnapshot - prevWallClockSnapshot) / threadNum);
    threadNum += threadNumChange; //Change thread number
//    INFO_LOGS("threadNum += %d = %d", threadNumChange, threadNum);
    cachedWallClockSnapshot = curWallClockCurSnapshot;
//    INFO_LOGS("Update cachedWallClockSnapshot = %lu", curWallClockCurSnapshot);

    //Update cached wallclocksnapshot to avoid first time lock
    wallclockSnapshot.store(curWallClockCurSnapshot, std::memory_order_release);
    threadUpdateLock.unlock();

    return logicalClock;
}

/**
 * API interface, use the following functions in hook
 */
inline uint64_t threadCreatedRecord(uint64_t &cachedWallClockSnapshot) {

    return updateLogicalClockAndThreadNum(1, cachedWallClockSnapshot);
}

inline uint64_t threadTerminatedRecord() {
    uint64_t tmp; //We do not need to update cachedWallClockSnapshot at exit
    return updateLogicalClockAndThreadNum(-1, tmp);
}

inline uint64_t calcCurrentLogicalClock(uint64_t &curWallClockSnapshot, uint64_t &cachedWallClockSnapshot) {
    uint64_t prevWallClockSnapshot = wallclockSnapshot.load(std::memory_order_acquire);
    //Updates performed by wallclockSnapshot must have been done. (Ensured by C++11 standard)
    uint64_t result = 0;
    if (prevWallClockSnapshot != cachedWallClockSnapshot) {
        //There is change to timestamp, get the lock and acquire again to ensure there are no data race on "threadNum" variable and "logicalClock" variable
        threadUpdateLock.lock();
        prevWallClockSnapshot = wallclockSnapshot.load(std::memory_order_acquire);
        //Updates performed by wallclockSnapshot must have been done. (Ensured by C++11 standard)
        cachedWallClockSnapshot = prevWallClockSnapshot;
        result = (curWallClockSnapshot - prevWallClockSnapshot) / threadNum + logicalClock;
        threadUpdateLock.unlock();
    } else {
        result = (curWallClockSnapshot - prevWallClockSnapshot) / threadNum + logicalClock;
//        INFO_LOGS("No thread creation/termination observed. API time is (%lu-%lu)/%u=%lu", curWallClockSnapshot,
//                  prevWallClockSnapshot, threadNum, (curWallClockSnapshot - prevWallClockSnapshot) / threadNum);

    }
    //Real time elapse + old logical clock value
    return result;
}

#endif
