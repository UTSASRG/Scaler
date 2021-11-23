#include <zconf.h>

#ifndef SCALER_PTHREADPARMEXTRACTOR_H
#define SCALER_PTHREADPARMEXTRACTOR_H

namespace scaler {
/**
 * WARNING: These functions are highly dependent on the assembly code inside ExtCallFuncHook
 * And thus very platform dependent.
 * Calling convention will impact these code
 */

/**
 * For assembly code
 * rdiLoc is the stack address that address the original rdi value
 * *rdiLoc is the rdi value and also the parameter passed
 * **rdiLoc is the pointer to the original parameter parsed (todo: change this to reference rather than pointer)
 *
 * For ptrace code
 * rdi is the value of rdi register already
 * *rdi is the variable value
 */

/**
 * pthread_t *__restrict __newthread,const pthread_attr_t *__restrict __attr,void *(*__start_routine) (void *), void *__restrict __arg) __THROWNL __nonnull ((1, 3)
 */
    inline void parm_pthread_create(pthread_t ***__newthread, void *rdiLoc) {
        //in edi
        *__newthread = (pthread_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_create(pthread_t *&__newthread, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        void *newThreadPtr = pmParser.readProcMem(reinterpret_cast<void *>(rdi), sizeof(pthread_t *));
        __newthread = *reinterpret_cast<pthread_t **>(newThreadPtr);
        free(newThreadPtr);
    }

/**
 * pthread_t __th, void **__thread_return
 */
    inline void parm_pthread_join(pthread_t **__th, void *rdiLoc) {
        //in edi
        *__th = (pthread_t *) ((char *) rdiLoc);
    }

    inline void parm_pthread_join(pthread_t &__th, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        __th = rdi;
    }

/**
 * pthread_t __th, void **__thread_return
 */
    inline void parm_pthread_tryjoin_np(pthread_t **__th, void *rdiLoc) {
        //in edi
        *__th = (pthread_t *) ((char *) rdiLoc);
    }

    inline void parm_pthread_tryjoin_np(pthread_t &__th, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_join(__th, pmParser, rdi);
    }

/**
 * pthread_t __th, void **__thread_return, const struct timespec *__abstime
 */
    inline void parm_pthread_timedjoin_np(pthread_t **__th, void *rdiLoc) {
        //in edi
        *__th = (pthread_t *) ((char *) rdiLoc);
    }

    inline void parm_pthread_timedjoin_np(pthread_t &__th, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_join(__th, pmParser, rdi);
    }

/**
 * pthread_t __th, void **__thread_return, clockid_t __clockid, const struct timespec *__abstime
 */
    inline void parm_pthread_clockjoin_np(pthread_t **__th, void *rdiLoc) {
        //in edi
        *__th = (pthread_t *) ((char *) rdiLoc);
    }

    inline void parm_pthread_clockjoin_np(pthread_t &__th, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_join(__th, pmParser, rdi);
    }

/**
 * pthread_mutex_t *__mutex
 */
    inline void parm_pthread_mutex_lock(pthread_mutex_t ***__mutex, void *rdiLoc) {
        //in edi
        *__mutex = (pthread_mutex_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_mutex_lock(pthread_mutex_t *&__th, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
//        void *result = pmParser.readProcMem(reinterpret_cast<void *>(rdi), sizeof(pthread_mutex_t *));
//        __th = *reinterpret_cast<pthread_mutex_t **>(result);
//        free(result);
        __th = reinterpret_cast<pthread_mutex_t *>(rdi);
    }

/**
 * pthread_mutex_t *__restrict __mutex, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_mutex_timedlock(pthread_mutex_t ***__mutex, void *rdiLoc) {
        //in edi
        *__mutex = (pthread_mutex_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_mutex_timedlock(pthread_mutex_t *&__th, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_mutex_lock(__th, pmParser, rdi);
    }

/**
 * pthread_mutex_t *__restrict __mutex, clockid_t __clockid, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_mutex_clocklock(pthread_mutex_t ***__mutex, void *rdiLoc) {
        //in edi
        *__mutex = (pthread_mutex_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_mutex_clocklock(pthread_mutex_t *&__th, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_mutex_lock(__th, pmParser, rdi);
    }

/**
 * pthread_mutex_t *__mutex
 */
    inline void parm_pthread_mutex_unlock(pthread_mutex_t ***__mutex, void *rdiLoc) {
        //in edi
        *__mutex = (pthread_mutex_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_mutex_unlock(pthread_mutex_t *&__th, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_mutex_lock(__th, pmParser, rdi);
    }

/**
 * pthread_rwlock_t *__rwlock
 */
    inline void parm_pthread_rwlock_rdlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_rdlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        __rwlock = reinterpret_cast<pthread_rwlock_t *>(rdi);
    }

/**
 * pthread_rwlock_t *__rwlock
 */
    inline void parm_pthread_rwlock_tryrdlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_tryrdlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }

/**
 * pthread_rwlock_t *__restrict __rwlock, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_rwlock_timedrdlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_timedrdlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }


    /**
 * pthread_rwlock_t *__restrict __rwlock, clockid_t __clockid, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_rwlock_clockrdlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_clockrdlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }


    /**
 * pthread_rwlock_t *__rwlock
 */
    inline void parm_pthread_rwlock_wrlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_wrlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }

/**
 * pthread_rwlock_t *__rwlock
 */
    inline void parm_pthread_rwlock_trywrlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_trywrlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }


    /**
 * pthread_rwlock_t *__restrict __rwlock, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_rwlock_timedwrlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_timedwrlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }


    /**
 * pthread_rwlock_t *__restrict __rwlock, clockid_t __clockid, const struct timespec *__restrict __abstime
 */
    inline void parm_pthread_rwlock_clockwrlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_clockwrlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }

/**
 * pthread_rwlock_t *__rwlock
 */
    inline void parm_pthread_rwlock_unlock(pthread_rwlock_t ***__rwlock, void *rdiLoc) {
        //in edi
        *__rwlock = (pthread_rwlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_rwlock_unlock(pthread_rwlock_t *&__rwlock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_rwlock_rdlock(__rwlock, pmParser, rdi);
    }

/**
 * pthread_cond_t *__cond
 */
    inline void parm_pthread_cond_signal(pthread_cond_t ***__cond, void *rdiLoc) {
        //in edi
        *__cond = (pthread_cond_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_cond_signal(pthread_cond_t *&__cond, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        __cond = reinterpret_cast<pthread_cond_t *>(rdi);
    }

    /**
 * pthread_cond_t *__cond
 */
    inline void parm_pthread_cond_broadcast(pthread_cond_t ***__cond, void *rdiLoc) {
        //in edi
        *__cond = (pthread_cond_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_cond_broadcast(pthread_cond_t *&__cond, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        parm_pthread_cond_signal(__cond, pmParser, rdi);
    }

    /**
 * pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex
 */
    inline void
    parm_pthread_cond_wait(pthread_cond_t ***__cond, pthread_mutex_t ***__mutex, void *rdiLoc, void *rsiLoc) {
        //in edi
        *__cond = (pthread_cond_t **) ((char *) rdiLoc);
        *__mutex = (pthread_mutex_t **) ((char *) rsiLoc);

    }

    inline void
    parm_pthread_cond_wait(pthread_cond_t *&__cond, pthread_mutex_t *&__mutex, PmParser_Linux &pmParser, int64_t rdi,
                           int64_t rsi) {
        __cond = *reinterpret_cast<pthread_cond_t **>(rdi);
        __mutex = *reinterpret_cast<pthread_mutex_t **>(rsi);
    }

/**
 * pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex, const struct timespec *__restrict __abstime
 */
    inline void
    parm_pthread_cond_timedwait(pthread_cond_t ***__cond, pthread_mutex_t ***__mutex, void *rdiLoc, void *rsiLoc) {
        //in edi
        *__cond = (pthread_cond_t **) ((char *) rdiLoc);
        *__mutex = (pthread_mutex_t **) ((char *) rsiLoc);

    }

    inline void
    parm_pthread_cond_timedwait(pthread_cond_t *&__cond, pthread_mutex_t *&__mutex, PmParser_Linux &pmParser,
                                int64_t rdi, int64_t rsi) {
        //in edi
        parm_pthread_cond_wait(__cond, __mutex, pmParser, rdi, rsi);
    }

/**
 * pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex, __clockid_t __clock_id, const struct timespec *__restrict __abstime
 */
    inline void
    parm_pthread_cond_clockwait(pthread_cond_t ***__cond, pthread_mutex_t ***__mutex, void *rdiLoc, void *rsiLoc) {
        //in edi
        *__cond = (pthread_cond_t **) ((char *) rdiLoc);
        *__mutex = (pthread_mutex_t **) ((char *) rsiLoc);
    }

    inline void
    parm_pthread_cond_clockwait(pthread_cond_t *&__cond, pthread_mutex_t *&__mutex, PmParser_Linux &pmParser,
                                int64_t rdi, int64_t rsi) {
        parm_pthread_cond_wait(__cond, __mutex, pmParser, rdi, rsi);
    }

/**
 * pthread_spinlock_t *__lock
 */
    inline void parm_pthread_spin_lock(pthread_spinlock_t ***__lock, void *rdiLoc) {
        //in edi
        *__lock = (pthread_spinlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_spin_lock(pthread_spinlock_t *&__lock, PmParser_Linux &pmParser, int64_t rdi) {
        //in edi
        __lock = reinterpret_cast<pthread_spinlock_t *>(rdi);
    }

/**
 * pthread_spinlock_t *__lock
 */
    inline void parm_pthread_spin_trylock(pthread_spinlock_t ***__lock, void *rdiLoc) {
        //in edi
        *__lock = (pthread_spinlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_spin_trylock(pthread_spinlock_t *&__lock, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_spin_lock(__lock, pmParser, rdi);
    }

/**
 * pthread_spinlock_t *__lock
 */
    inline void parm_pthread_spin_unlock(pthread_spinlock_t ***__lock, void *rdiLoc) {
        //in edi
        *__lock = (pthread_spinlock_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_spin_unlock(pthread_spinlock_t *&__lock, PmParser_Linux &pmParser, int64_t rdi) {
        parm_pthread_spin_lock(__lock, pmParser, rdi);
    }

/**
 * pthread_barrier_t *__barrier
 */
    inline void parm_pthread_barrier_wait(pthread_barrier_t ***__barrier, void *rdiLoc) {
        //in edi
        *__barrier = (pthread_barrier_t **) ((char *) rdiLoc);
    }

    inline void parm_pthread_barrier_wait(pthread_barrier_t *&__barrier, PmParser_Linux &pmParser, int64_t rdi) {
        __barrier = reinterpret_cast<pthread_barrier_t *>(rdi);
    }
}
#endif //SCALER_PTHREADPARMEXTRACTOR_H