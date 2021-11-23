#if !defined(DOUBLETAKE_GLOBALINFO_H)
#define DOUBLETAKE_GLOBALINFO_H

/*
 * @file   globalinfo.h
 * @brief  some global information about this system, by doing this, we can avoid multiple copies.
 *         Also, it is very important to utilize this to cooperate multiple threads since
 * pthread_kill
 *         actually can not convey additional signal value information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "log.hh"
#include "real.hh"
#include "threadstruct.hh"
#include "internalsyncs.hh"

#ifndef PTHREADEXIT_CODE
#define PTHREADEXIT_CODE 2230
#endif

typedef enum SystemPhase : unsigned long {
  E_SYS_INIT,        // Initialization phase
  E_SYS_NORMAL_EXECUTION,
  E_SYS_REEXECUTION,
  E_SYS_EPOCH_END,   // We are just before commit.
  E_SYS_EPOCH_BEGIN, // We have to start a new epoch when no overflow.
  E_SYS_ROLLBACK,   // Rollback phase //Hongyu
  E_SYS_TO_ROLLBACK, // This is a call to rollback, setting up by the coordinator thread  
} eSystemPhase;

typedef enum SystemAction : unsigned long {
  E_SYSACTION_TO_NONE, 
  E_SYSACTION_TO_STOPALL, 
  E_SYSACTION_TO_ROLLBACK,
} eSystemAction;
/* 
   The state of the system. 
   E_SYS_NORMAL_EXECUTION, 
   E_SYS_REEXECUTION,
   E_SYS_EPOCH_END,
   E_SYS_EPOCH_BEGIN,
  
   E_SYS_TO_ROLLBACK 
 */

// We only need to maintain a phase information, but not multiple variables.
extern bool g_hasRollbacked;
extern bool g_bugReproduced;
extern int g_rbNumber;
#ifdef TESTSEGV 
extern int g_rbInSegvNumber;
extern unsigned long g_rbInSegvStartTime;
extern unsigned long g_rbInSegvEpochStartTime;
extern unsigned long trace_inst_count;
#endif
extern eSystemPhase  g_phase;
extern eSystemAction g_action; // It is set up when we want to rollback! NEW! 
extern int g_waiters;
extern int g_waitersTotal;

extern pthread_cond_t g_condCommitter;
extern pthread_cond_t g_condWaiters;
extern pthread_mutex_t g_mutex;
extern pthread_spinlock_t g_spinlock; //Hongyu
extern pthread_mutex_t g_mutexSignalhandler;
extern int waitingFenceThreads;
extern bool waitingFence;

inline unsigned long getCurrentTime(){
  unsigned long retval = 0;
  struct timespec ts;
  if(Real::clock_gettime(CLOCK_REALTIME, &ts) == 0){
    retval = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  }
  return retval;
}

inline void global_lock() { Real::pthread_mutex_lock(&g_mutex); }
inline void global_spinlock() { pthread_spin_lock(&g_spinlock); } //Hongyu

inline void global_unlock() { Real::pthread_mutex_unlock(&g_mutex); }
inline void global_spinunlock() { pthread_spin_unlock(&g_spinlock); } //Hongyu

inline void global_lockInsideSignalhandler() { Real::pthread_mutex_lock(&g_mutexSignalhandler); }

inline void global_unlockInsideSignalhandler() {
  Real::pthread_mutex_unlock(&g_mutexSignalhandler);
}

inline void global_initialize() {
  g_hasRollbacked = false;
  g_bugReproduced = false;
  g_rbNumber = 0;
#ifdef TESTSEGV 
  g_rbInSegvNumber = 0;
  g_rbInSegvStartTime = 0;
  g_rbInSegvEpochStartTime = 0;
  trace_inst_count = 0;
#endif
  g_phase = E_SYS_INIT;
  g_action = E_SYSACTION_TO_NONE; 
  waitingFence = true; //Hongyu
  waitingFenceThreads=0;

  Real::pthread_mutex_init(&g_mutex, NULL);
  pthread_spin_init(&g_spinlock, PTHREAD_PROCESS_PRIVATE); //Hongyu
  Real::pthread_mutex_init(&g_mutexSignalhandler, NULL);
  Real::pthread_cond_init(&g_condCommitter, NULL);
  Real::pthread_cond_init(&g_condWaiters, NULL);
}

inline void global_setNormalExecution() {
  g_phase = E_SYS_NORMAL_EXECUTION; 
}

inline void global_setEpochEnd() {
  g_phase = E_SYS_EPOCH_END;
  g_waitersTotal=0;//reset waiters
}

inline int global_rbNumber() { return g_rbNumber; }

#ifdef TESTSEGV 
inline int global_rbSegvNumber() { return g_rbInSegvNumber; }
inline unsigned long global_rbSegvEpochStartTime() { return g_rbInSegvEpochStartTime; }
inline void set_global_rbSegvStartTime() { g_rbInSegvStartTime = getCurrentTime(); }
inline void set_global_rbSegvEpochStartTime() { g_rbInSegvEpochStartTime = getCurrentTime(); }
inline void global_addrbSegvNumber() { g_rbInSegvNumber++; }
#endif

inline bool global_isInitPhase() { return g_phase == E_SYS_INIT; }

inline bool global_isEpochEnd() { return g_phase == E_SYS_EPOCH_END; }

inline bool global_isRollback() { return g_phase == E_SYS_REEXECUTION; }

inline bool global_isEpochBegin() { return g_phase == E_SYS_EPOCH_BEGIN; }

inline void global_setRollback() {
  g_hasRollbacked = true;
  g_phase = E_SYS_REEXECUTION;
  g_action = E_SYSACTION_TO_ROLLBACK;
  g_rbNumber++;
}

//FIXME TP: whether this can cause the deadlock or not?
inline void setStopallAction() {
  //global_lock();
  global_spinlock();
  if(g_action == E_SYSACTION_TO_STOPALL) {
    // If aother thread has already notified all threads to stop,
    // then I am the second one and should not be the coordinator thread any more.
	  lock_thread(current);
	  current->action = E_THREAD_ACTION_EPOCH_SKIP;
	  unlock_thread(current);
  }
  else {
	  g_action = E_SYSACTION_TO_STOPALL;
  }
  global_spinunlock();
  //global_unlock();
}

inline bool global_isStopStatus() {
  return g_action == E_SYSACTION_TO_STOPALL;
}

inline bool global_hasRollbacked() { return g_hasRollbacked; }

inline bool global_toRollback() { return g_action == E_SYSACTION_TO_ROLLBACK; }

inline void global_wakeup() {
  // Wakeup all other threads.
  Real::pthread_cond_broadcast(&g_condWaiters);
}


inline void global_epochBegin() {
  //global_lock();
  global_spinlock();
  g_phase = E_SYS_EPOCH_BEGIN;
  g_action = E_SYSACTION_TO_NONE; 
  //global_unlock();
  global_spinunlock();
}

// Waiting for the stops of threads, no need to hold the lock.
inline void global_waitingThreads(int totalwaiters) {
  g_waitersTotal = totalwaiters;
}

// Waiting for the stops of threads, no need to hold the lock.
inline void global_waitThreadsStops(int totalwaiters) {
  global_lockInsideSignalhandler();
  g_waitersTotal = totalwaiters;
  //    PRINF("During waiting: g_waiters %d g_waitersTotal %d\n", g_waiters, g_waitersTotal);
  while(g_waiters != g_waitersTotal) {
    Real::pthread_cond_wait(&g_condCommitter, &g_mutexSignalhandler);
  }
  global_unlockInsideSignalhandler();
}

inline void global_checkWaiters() { 
//	PRINT("thread %d, global_checkWaiters is %d\n", current->index, g_waiters);
	assert(g_waiters == 0); 
}

// Notify the commiter and wait on the global conditional variable
inline void global_waitForNotification() {
  assert(global_isEpochEnd() == true);

  //    printf("waitForNotification, waiters is %d at thread %p\n", g_waiters, pthread_self());
  global_lockInsideSignalhandler();
  //PRINF("waitForNotification g_waiters %d totalWaiters %d\n", g_waiters, g_waitersTotal);
  g_waiters++;

	// Wakeup the committer
  if(g_waiters == g_waitersTotal) {
    Real::pthread_cond_signal(&g_condCommitter);
  }

  PRINF("thread %d waitForNotification\n", current->index);
  // Only waken up when it is not the end of epoch anymore.
  while(global_isEpochEnd()) {
    PRINF("thread %d waitForNotification before waiting again\n", current->index);
    Real::pthread_cond_wait(&g_condWaiters, &g_mutexSignalhandler);
    PRINF("thread %d waitForNotification after waken up. isEpochEnd() %d \n", current->index, global_isEpochEnd());
  }

  g_waiters--;

  if(g_waiters == 0) {
    Real::pthread_cond_signal(&g_condCommitter);
  }

  global_unlockInsideSignalhandler();
}

inline void global_waitfence(){
  PRINF("thread %d wait fence, num %d\n", current->index, g_waitersTotal);
  global_lockInsideSignalhandler();

  while(waitingFence != true) {
    Real::pthread_cond_wait(&g_condWaiters, &g_mutexSignalhandler);
  }

  waitingFenceThreads++;

  if(waitingFenceThreads >= g_waitersTotal) {
    waitingFence = false;
    Real::pthread_cond_broadcast(&g_condWaiters);
  } 
  else {
    assert(waitingFence == true);

    while(waitingFence == true) {
      //markThreadCondwait(g_condWaiters, E_THREAD_COND_WAITING);
      Real::pthread_cond_wait(&g_condWaiters, &g_mutexSignalhandler);
      //unmarkThreadCondwait(g_mutexSignalhandler);
    }
  }

  waitingFenceThreads--;	

  if(waitingFenceThreads == 0) {
    waitingFence = true;
    Real::pthread_cond_broadcast(&g_condWaiters);
  }

  //setThreadSafe();
  global_unlockInsideSignalhandler();
  PRINF("thread %d leave fence\n", current->index);
}

inline void global_AtmoicFence(){

  unsigned int waitingThreads;

  while(__atomic_load_n(&waitingFence, __ATOMIC_SEQ_CST) != true) {
    __asm__ __volatile__ ("mfence");
  }

  waitingThreads = __atomic_add_fetch(&waitingFenceThreads, 1, __ATOMIC_SEQ_CST);
  __asm__ __volatile__ ("mfence");
  if(waitingThreads == g_waitersTotal) {

    __atomic_clear(&waitingFence, __ATOMIC_SEQ_CST);
    __asm__ __volatile__ ("mfence");

    waitingThreads = __atomic_sub_fetch(&waitingFenceThreads, 1, __ATOMIC_SEQ_CST);  
  }
  else {
    while(__atomic_load_n(&waitingFence, __ATOMIC_SEQ_CST) == true) {
      __asm__ __volatile__ ("mfence");
    }

    waitingThreads = __atomic_sub_fetch(&waitingFenceThreads, 1, __ATOMIC_SEQ_CST);  
  }
  __asm__ __volatile__ ("mfence");

  if(waitingThreads == 0) {
    __atomic_store_n(&waitingFence, true, __ATOMIC_SEQ_CST);
    __asm__ __volatile__ ("mfence");
  }

}

#endif
