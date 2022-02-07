#include <pthread.h>
#include <cstdio>

#define MAGIC_NUMBER1 0x11
#define MAGIC_NUMBER2 0x22
#define MAGIC_NUMBER3 0x33
#define MAGIC_NUMBER4 0x44
#define MAGIC_NUMBER5 0x55
#define MAGIC_NUMBER6 0x66
#define MAGIC_NUMBER7 0x77
#define MAGIC_NUMBER8 0x88
#define MAGIC_NUMBER9 0x99


/**
 * This demo is meant to be used with a debugger. Rather than execute normally
 * @return
 */

int main() {
    start:

    lbl_pthread_create:
    pthread_create((pthread_t *) MAGIC_NUMBER1, (pthread_attr_t *) MAGIC_NUMBER2, (void *(*)(void *)) (MAGIC_NUMBER4),
                   (void *) MAGIC_NUMBER5);
    lbl_pthread_join:
    //pthread_join(pthread_t * *__th);
    pthread_join((pthread_t) MAGIC_NUMBER1, (void **) (MAGIC_NUMBER2));
    lbl_pthread_tryjoin_np:
    pthread_tryjoin_np((pthread_t) MAGIC_NUMBER1, (void **) (MAGIC_NUMBER2));
    lbl_pthread_timedjoin_np:
    pthread_timedjoin_np((pthread_t) MAGIC_NUMBER1, (void **) (MAGIC_NUMBER2), (timespec *) MAGIC_NUMBER3);
    lbl_pthread_clockjoin_np:
//    pthread_clockjoin_np((pthread_t) MAGIC_NUMBER1, (void **) MAGIC_NUMBER2, (clockid_t) MAGIC_NUMBER3,
//                         (timespec *) MAGIC_NUMBER4);
    lbl_pthread_mutex_lock:
    pthread_mutex_lock((pthread_mutex_t *) MAGIC_NUMBER1);
    lbl_pthread_mutex_timedlock:
    pthread_mutex_timedlock((pthread_mutex_t *) MAGIC_NUMBER1, (timespec *) MAGIC_NUMBER2);
    lbl_pthread_mutex_clocklock:
//    pthread_mutex_clocklock((pthread_mutex_t *) MAGIC_NUMBER1, (clockid_t) MAGIC_NUMBER2, (timespec *) MAGIC_NUMBER3);
    lbl_pthread_mutex_unlock:
    pthread_mutex_unlock((pthread_mutex_t *) MAGIC_NUMBER1);
    lbl_pthread_rwlock_rdlock:
    pthread_rwlock_rdlock((pthread_rwlock_t *) MAGIC_NUMBER1);
    lbl_pthread_rwlock_tryrdlock:
    pthread_rwlock_tryrdlock((pthread_rwlock_t *) MAGIC_NUMBER1);
    lbl_pthread_rwlock_timedrdlock:
    pthread_rwlock_timedrdlock((pthread_rwlock_t *) MAGIC_NUMBER1, (timespec *) MAGIC_NUMBER2);
    lbl_pthread_rwlock_clockrdlock:
//    pthread_rwlock_clockrdlock((pthread_rwlock_t *) MAGIC_NUMBER1, (clockid_t) MAGIC_NUMBER2,
//                               (timespec *) MAGIC_NUMBER3);
    lbl_pthread_rwlock_wrlock:
    pthread_rwlock_wrlock((pthread_rwlock_t *) MAGIC_NUMBER1);
    lbl_pthread_rwlock_trywrlock:
    pthread_rwlock_trywrlock((pthread_rwlock_t *) MAGIC_NUMBER1);
    lbl_pthread_rwlock_timedwrlock:
    pthread_rwlock_timedwrlock((pthread_rwlock_t *) MAGIC_NUMBER1, (timespec *) MAGIC_NUMBER2);
    lbl_pthread_rwlock_clockwrlock:
//    pthread_rwlock_clockwrlock((pthread_rwlock_t *) MAGIC_NUMBER1, (clockid_t) MAGIC_NUMBER2,
//                               (timespec *) MAGIC_NUMBER3);
    lbl_pthread_rwlock_unlock:
    pthread_rwlock_unlock((pthread_rwlock_t *) MAGIC_NUMBER1);
    lbl_pthread_cond_signal:
    pthread_cond_signal((pthread_cond_t *) MAGIC_NUMBER1);
    lbl_pthread_cond_broadcast:
    pthread_cond_broadcast((pthread_cond_t *) MAGIC_NUMBER1);
    lbl_pthread_cond_wait:
    pthread_cond_wait((pthread_cond_t *) MAGIC_NUMBER1, (pthread_mutex_t *) MAGIC_NUMBER2);
    lbl_pthread_cond_timedwait:
    pthread_cond_timedwait((pthread_cond_t *) MAGIC_NUMBER1, (pthread_mutex_t *) MAGIC_NUMBER2,
                           (timespec *) MAGIC_NUMBER3);
    lbl_pthread_cond_clockwait:
//    pthread_cond_clockwait((pthread_cond_t *) MAGIC_NUMBER1, (pthread_mutex_t *) MAGIC_NUMBER2,
//                           (__clockid_t) MAGIC_NUMBER3, (timespec *) MAGIC_NUMBER4);
    lbl_pthread_spin_lock:
    pthread_spin_lock((pthread_spinlock_t *) MAGIC_NUMBER1);
    lbl_pthread_spin_trylock:
    pthread_spin_trylock((pthread_spinlock_t *) MAGIC_NUMBER1);
    lbl_pthread_spin_unlock:
    pthread_spin_unlock((pthread_spinlock_t *) MAGIC_NUMBER1);
    lbl_pthread_barrier_wait:
    pthread_barrier_wait((pthread_barrier_t *) MAGIC_NUMBER1);
    return 0;
}