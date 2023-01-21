#ifndef SCALER_SPINLOCK_H
#define SCALER_SPINLOCK_H

#include <immintrin.h>
#include <util/tool/Logging.h>
/**
 * This lock is non-reentrant implemented using atomic operations
 */

inline void atomicSpinLock(bool &lock) {
//    INFO_LOG("Atomic spinlock");
    while (true) {
        if (!__atomic_test_and_set(&lock, __ATOMIC_ACQUIRE)) {
            break;
        }
        bool lockOccupied = true;
        while (lockOccupied) {
            __atomic_load(&lock, &lockOccupied, __ATOMIC_RELAXED);
            _mm_pause();
        }
    }
}

inline void atomicSpinUnlock(bool &lock) {
//    INFO_LOG("Atomic spinunlock");
    bool lockOccupied = false;
    __atomic_store(&lock, &lockOccupied, __ATOMIC_RELEASE);
}

#endif //SCALER_SPINLOCK_H
