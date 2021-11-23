#include <semaphore.h>

#ifndef SCALER_SEMAPHOREPARMEXTRACTOR_H
#define SCALER_SEMAPHOREPARMEXTRACTOR_H

/**
 * WARNING: These functions are highly dependent on the assembly code inside ExtCallFuncHook
 * And thus very platform dependent.
 * Calling convention will impact these code
 */

namespace scaler {
/**
 * sem_t *__sem
 */
    inline void parm_sem_wait(sem_t ***__sem, void *rdiLoc) {
        //in edi
        *__sem = (sem_t **) ((char *) rdiLoc);
    }

    inline void parm_sem_wait(sem_t *&__sem, PmParser_Linux &pmParser, int64_t rdi) {
        void *result = pmParser.readProcMem(reinterpret_cast<void *>(rdi), sizeof(sem_t *));
        __sem = *reinterpret_cast<sem_t **>(result);
        free(result);
    }

/**
 * sem_t *__restrict __sem,
 */
    inline void parm_sem_timedwait(sem_t ***__sem, void *rdiLoc) {
        //in edi
        parm_sem_wait(__sem, rdiLoc);
    }

    inline void parm_sem_timedwait(sem_t *&__sem, PmParser_Linux &pmParser, int64_t rdi) {
        parm_sem_wait(__sem, pmParser, rdi);
    }

/**
 * sem_t *__restrict __sem
 */
    inline void parm_sem_clockwait(sem_t ***__sem, void *rdiLoc) {
        //in edi
        parm_sem_wait(__sem, rdiLoc);
    }

    inline void parm_sem_clockwait(sem_t *&__sem, PmParser_Linux &pmParser, int64_t rdi) {
        parm_sem_wait(__sem, pmParser, rdi);
    }

/**
 * sem_t *__sem
 */
    inline void parm_sem_trywait(sem_t ***__sem, void *rdiLoc) {
        //in edi
        parm_sem_wait(__sem, rdiLoc);
    }

    inline void parm_sem_trywait(sem_t *&__sem, PmParser_Linux &pmParser, int64_t rdi) {
        parm_sem_wait(__sem, pmParser, rdi);
    }


/**
 * sem_t *__sem
 */
    inline void parm_sem_post(sem_t ***__sem, void *rdiLoc) {
        //in edi
        parm_sem_wait(__sem, rdiLoc);
    }

    inline void parm_sem_post(sem_t *&__sem, PmParser_Linux &pmParser, int64_t rdi) {
        parm_sem_wait(__sem, pmParser, rdi);
    }


}
#endif //SCALER_SEMAPHOREPARMEXTRACTOR_H




