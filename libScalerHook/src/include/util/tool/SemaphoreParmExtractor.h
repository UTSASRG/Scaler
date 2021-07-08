#include <semaphore.h>

#ifndef SCALER_SEMAPHOREPARMEXTRACTOR_H
#define SCALER_SEMAPHOREPARMEXTRACTOR_H

/**
 * WARNING: These functions are highly dependent on the assembly code inside ExtCallFuncHook
 * And thus very platform dependent.
 * Calling convention will impact these code
 */

/**
 * sem_t *__sem
 */
inline void parm_sem_wait(sem_t ***__sem, void *rdiLoc) {
    //in edi
    *__sem = (sem_t **) ((char *) rdiLoc);
}

/**
 * sem_t *__restrict __sem,
 */
inline void parm_sem_timedwait(sem_t ***__sem, void *rdiLoc) {
    //in edi
    parm_sem_wait(__sem, rdiLoc);
}

/**
 * sem_t *__restrict __sem
 */
inline void parm_sem_clockwait(sem_t ***__sem, void *rdiLoc) {
    //in edi
    parm_sem_wait(__sem, rdiLoc);

}

/**
 * sem_t *__sem
 */
inline void parm_sem_trywait(sem_t ***__sem, void *rdiLoc) {
    //in edi
    parm_sem_wait(__sem, rdiLoc);
}

/**
 * sem_t *__sem
 */
inline void parm_sem_post(sem_t ***__sem, void *rdiLoc) {
    //in edi
    parm_sem_wait(__sem, rdiLoc);
}


#endif //SCALER_SEMAPHOREPARMEXTRACTOR_H




