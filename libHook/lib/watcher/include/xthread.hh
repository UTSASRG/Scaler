#if !defined(DOUBLETAKE_XTHREAD_H)
#define DOUBLETAKE_XTHREAD_H

/*
 * @file   xthread.h
 * @brief  Handling different kinds of synchronizations, like thread creation and exit,
 *         lock, conditional variables and barriers.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

#include <new>

#include "globalinfo.hh"
#include "internalheap.hh"
#include "internalsyncs.hh"
#include "list.hh"
#include "log.hh"
#include "real.hh"
#include "sysrecord.hh"
#include "selfmap.hh"
#include "synceventlist.hh"
#include "threadinfo.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xcontext.hh"
#include "xdefines.hh"
#include "xsync.hh"
#include "xrun.hh"

#ifdef DUMP_LOG_TODISK
#define MAX_FILENAME_LEN 1024 
extern char * program_invocation_name;
#endif

class xthread {
  // After the first pointer, we will keep 
  // a data structure to keep all information about this barrier
  // barrier is typically 32 bytes, thus, it should be enough.  
  class BarrierInfo {
  public:
    int maxThreads;
    int waitingThreads;
    bool isArrivalPhase;
  };

public:
  xthread() : _sync(xsync::getInstance())
  {}

  // Actually, it is not an actual singleton.
  // Every process will have one copy. They can be used
  // to hold different contents specific to different threads.
  static xthread& getInstance() {
    static char buf[sizeof(xthread)];
    static xthread* xthreadObject = new (buf) xthread();
    return *xthreadObject;
  }

  void initialize() {
    threadinfo::getInstance().initialize();

    // Initialize the syncmap and threadmap.
    _sync.initialize();
    threadmap::getInstance().initialize();

    // Initialize the global list for spawning operations
    void* ptr = ((void*)InternalHeap::getInstance().malloc(sizeof(SyncEventList)));
    _spawningList = new (ptr) SyncEventList(NULL, E_SYNC_SPAWN, 0);

    // Register the first thread
    registerInitialThread();
    PRINF("Done with thread initialization");
  }

  void reInitialize() {

    threadinfo::getInstance().initialize();
    threadmap::getInstance().initialize();

    // Initialize the global list for spawning operations
    _spawningList->cleanup();

    // Register the first thread
    registerInitialThread();
    PRINF("Done with thread reInitialization");
  }

  void finalize() {
    //FIXME SEM
    //destroyAllSemaphores(); 
  }

  inline void cleanupSpawningList(){
    _spawningList->cleanup();
  }

  // Register initial thread
  inline void registerInitialThread() {
    int tindex = allocThreadIndex();

    if (tindex == -1) {
      return;
    }

    thread_t* tinfo = getThreadInfo(tindex);

    // Set the current to corresponding tinfo.
    current = tinfo;
    current->joiner = NULL;
    current->index = tindex;
    current->parent = NULL;

    insertAliveThread(current, pthread_self());

    // Setup tindex for initial thread.
    threadRegister(true);
    current->isNewlySpawned = false;
  }

  /// Handling the specific thread event.
  void thread_exit(void*) {
    // FIXME later.
    //  abort();
  }

  // In order to improve the speed, those spawning operations will do in
  // a batched way. Everything else will be stopped except this spawning process.
  // All newly spawned children will also wait for the notification of the parent.
  // SO only the first time, the thread will wait on fence.
  // To guarantee the correctness, those newly spawned threads will issue
  // an epochBegin() to discard those possibly polluted pages.
  // For the parent, because no one is running when spawnning, so there is no
  // need to call epochBegin().
  int thread_create(pthread_t* tid, const pthread_attr_t* attr, threadFunction* fn, void* arg) {
    int tindex;
    int result;
  
  // Lock and record
    PRINF("thread %d is before thread_create now\n", current->index);
    if(!global_isRollback()) {
    
      global_spinlock(); //Hongyu
   
      // Allocate a global thread index for current thread.
      tindex = allocThreadIndex();

      // Typeicall, thread index should be larger than 0, where only the initial thread will be 0.
      assert(tindex > 0);

      // WRAP up the actual thread function and setup correponding thread_t structure. 
      // Now we don't worry about the race condition and there is no need to acquire the lock.
      thread_t* children = getThreadInfo(tindex);

      children->isDetached = false;
      if(attr) {
        int detachState;
        pthread_attr_getdetachstate(attr, &detachState);

        // Check whether the thread is detached or not?
        if(detachState == PTHREAD_CREATE_DETACHED) {
          children->isDetached = true;
        }
      }

      children->parent = current;
      children->index = tindex;
      children->startRoutine = fn;
      children->startArg = arg;
      children->status = E_THREAD_STARTING;
      children->hasJoined = false;
      children->lockcounter = 0;
      
      // Now we set the joiner to NULL before creation.
      // It is impossible to let newly spawned child to set this correctly since
      // the parent may already sleep on that.
      children->joiner = NULL;

      PRINF("thread creation with index %d\n", tindex);
      // Record this spawning event, but not system calls inside creations.
      // since threads will be kept to the end of the epoch and no need to create again during re-execution
      disableCheck();
      result = Real::pthread_create(tid, attr, xthread::startThread, (void*)children);
      enableCheck();
      if(result != 0) {
        PRWRN("thread creation failed with errno %d -- %s\n", errno, strerror(errno));
        
        // Maybe we should not exit. But typically this branch will never reach.
        // Not a big deal here. 
        Real::exit(-1);
      }

      // Record spawning event since it is always successful
      _spawningList->recordSyncEvent(current, E_SYNC_SPAWN, result);
      _sysrecord.recordCloneOps(result, *tid);

      // Insert this thread into the alive list. 
      // No need to hold the lock of its children
      insertAliveThread(children, *tid);

      // Waiting for the finish of registering children thread to proceed.
      lock_thread(children);
      while(children->status == E_THREAD_STARTING) {
        wait_thread(children);
      }
      unlock_thread(children);

#ifdef GET_CHARECTERISTICS
   __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
#endif

      global_spinunlock(); //Hongyu
    } else {
      if(_sync.peekSyncEvent(_spawningList, &result) != true) {
        xrun::getInstance().rollbackDuringReexecution();  
      }
      PRINF("process %d is before thread_create, result %d\n", current->index, result);

      _sysrecord.getCloneOps(tid, &result);

      assert(result == 0);
      PRINF("process %d in creation, result %d\n", current->index, result);
      
      // Wait the semaphore to actually wake up its child.  
      waitSignal(_spawningList);
        
      PRINF("process %d is after waitsemaphore, thread %lx\n", current->index, *tid);

      // Wakeup the correponding child thread so that it can move on.
      thread_t* thread = getThreadByTid(*tid);

      PRINF("wakeup thread %d at thread create, status %d\n", thread->index, thread->status);
      // Wakeup corresponding thread
      thread->joiner = NULL;
      thread->status = E_THREAD_RUNNING;
      thread->action = E_THREAD_ACTION_ROLLBACK;

      // No matter where its child park on, we use the same funtion to wake up
      // FIXME if thead is waiting on sigwait syscall, we should use SIGPWR to wake up  
      signal_waiting_thread(thread);
      
      // Update the events.
      PRINF("#############process %d before updateSyncEvent now\n", current->index);
      updateSyncEvent(_spawningList);
      PRINF("#############process %d after updateSyncEvent now\n", current->index);
    }
    
    return result;
  }

  inline void setThreadWaiting(pthread_cond_t * cond, thrStatus status) {
    lock_thread(current);
    current->condwait = cond;
    current->status = status;
    unlock_thread(current);
  }

  // Check the statuses and possible actions after wakenup
  inline void checkAtThreadWakenup(pthread_mutex_t * mutex, bool * toCheckpoint) {
    lock_thread(current);

    // Cleanup this thread based on the action
    switch(current->action) {
      case E_THREAD_ACTION_NONE:
      {
        current->condwait = NULL;
        current->status = E_THREAD_RUNNING;
        unlock_thread(current);
        break;
      }
      
      // Stop this thread on its thread-wise conditonal variable 
      case E_THREAD_ACTION_STOP:
      {
        //RELEASELOCK
        //xthread::getInstance().releaseHoldingLocks(mutex);
//#warning "how to release lock and get lock again"
        xthread::getInstance().releaseHoldingLocks();

        stopCurrentThreadWithoutCheckpoint();
        // Fall through to the next one
      }

      case E_THREAD_ACTION_CHECKPOINT:
      {
        assert(toCheckpoint != NULL); 
        *toCheckpoint = true;

        // We should set the action back to normal since we already got the action above.
        current->action = E_THREAD_ACTION_NONE;

        // Notify the coordinator thread to proceed
       // signal_thread_epochbegin(current);

        unlock_thread(current);
        break;
      }  


      // Rollback immediately if required
      case E_THREAD_ACTION_ROLLBACK: 
      { 
        current->condwait = NULL;
        unlock_thread(current);
    
        if(mutex) {
          Real::pthread_mutex_unlock(mutex);
        }

        // Never return
        rollbackCurrent();
        break;
      }

      default:
        unlock_thread(current);
        // How should we handle E_THREAD_ACTION_EPOCH_SKIP???
        // FIXME, Tongping
        break;
    }
        
    
  }

  /// @brief Wait for a thread to exit.
  inline int thread_join(pthread_t joinee, void** result) {

  //treate it as checkpoint
  //invokeCommit();
#ifdef GET_CHARECTERISTICS
   __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
#endif
    // Check whether we should stop or not 
    checkStopStatus();

    thread_t* thread = NULL;
    bool toCheckpoint = false;

    // Try to check whether thread is empty or not?
    thread = getThreadByTid(joinee);
    assert(thread != NULL);

    PRINF("thread %d is joining thread %d\n", current->index, thread->index);
join_again:
    // Checkpoint correspondingly if the current thread is waken up because of entering next epoch
    // Thus, the checkpoint state will hold the joinee's lock only
    if(toCheckpoint == true) {
      toCheckpoint = false;
      saveContext();
    }

    lock_thread(thread);
    PRINF("thread %d is joining thread %d with status %d\n", current->index, thread->index, thread->status);
    while(thread->status != E_THREAD_WAITFOR_REAPING) {
      
      // Set the joiner to current thread
      thread->joiner = current;

      // Wait for the joinee or the coordinator to wake me up
      // There is no problem to acquire another lock here, which will not cause 
      // deadlock problem.
      setThreadWaiting(&thread->condjoiner, E_THREAD_JOINING);
      // Actually wait for the joinee's notification 
      wait_thread_joiner(thread);

      // reset thread joiner status
      thread->joiner = NULL;

      // The following function may not return if it is notified to rollback  
      checkAtThreadWakenup(&thread->mutex, &toCheckpoint);
      
      if(toCheckpoint){
        unlock_thread(thread);
        goto join_again;
      }
    }

    // Now mark this thread's status so that the thread can be reaped.
    thread->hasJoined = true;

    // Actually, we should get the result from corresponding thread
    if(result) {
      *result = thread->result;
    }

    // Now we unlock and proceed
    unlock_thread(thread);

    // Defer the reaping of this thread for memory deterministic usage.
    if(deferSync((void *)thread, E_SYNCVAR_THREAD)) {
      PRINF("Before reap dead threads!!!!\n");
      // deferSync returns TRUE if dead threads are larger than xdefines::MAX_REAPABLE_THREADS
      // Thus, we have to reap all dead threads in order to free up the heapids and thread_t.
      invokeCommit();
      PRINF("After reap dead threads!!!!\n");
    }
 
    return 0;
  }

  /// @brief Detach a thread: just set the status, which is similar to the 
  /// the one with attributes as detached.
  /// If a thread is detached, it should not be joined anymore.
  inline int thread_detach(pthread_t thread) {
    thread_t* threadinfo = NULL;

    // Try to check whether thread is empty or not?
    threadinfo = getThreadByTid(thread);

    assert(threadinfo != NULL);

    lock_thread(threadinfo);
    threadinfo->isDetached = true;
    unlock_thread(threadinfo);

    return 0;
  }

  /// @brief Do a pthread_cancel: FIXME in the future.
  /// Currently, let's skip this operation. 
  inline int thread_cancel(pthread_t thread) {
    // We didn't handle pthread_cancel well. 
    // We should not issue cancel directly. We can only stop the current thread.
    thread_t* threadinfo = NULL;

    threadinfo = getThreadByTid(thread);
    if(threadinfo != NULL) {
      // Remove this thread from active list and release its thread_t structure. 
      // TODO: we try to maintain the cancelled thread in order to identically replay
      //       But it is a data race, when it is removed from threadmap and epochEnd happens
      //       Now we just treat this type of thread as normal exit. 
      //       Because we intercept thread_create.
      // deferSync((void *)threadinfo, E_SYNCVAR_THREAD);

      // set signal to per-thread data structure
      threadinfo->sig = SIGKILL;
    }
    return 0;
  }

  // A temporary implementation of pthread_kill. FIXME
  inline int thread_kill(pthread_t thread, int sig) {
    int retval = 0;

    PRINF("kill thread %lx with signal %d", thread, sig);
    if(sig == SIGQUIT || sig == SIGABRT || sig == SIGHUP || sig == SIGWINCH
       || sig == SIGKILL || sig == SIGTERM) {
      
      // Cleanup the status of these threads.
      thread_t* threadinfo = NULL;

      threadinfo = getThreadByTid(thread);
      assert(threadinfo != NULL);

      // Remove this thread from active list and release its thread_t structure. 
      deferSync((void *)threadinfo, E_SYNCVAR_THREAD);

      // set signal to per-thread data structure
      threadinfo->sig = SIGKILL;
    }
    else {      
      retval = Real::pthread_kill(thread, sig);
    }

    return retval; 
  }

  /// Save those actual mutex address in original mutex.
  int mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    // The synchronization here is totally broken: initializer should read state, then check
    // if init is needed. If so, then allocate and atomic compare exchange.

    int locktype = -1;
    if(attr) pthread_mutexattr_gettype(attr, &locktype);
    //fprintf(stderr, "mutex %p locktype is %d (%d)\n", mutex, locktype, PTHREAD_MUTEX_RECURSIVE);

    int result=0;
    if(!global_isRollback()) {
      // Allocate a mutex
      pthread_mutex_t* real_mutex =
          (pthread_mutex_t*)allocRealSyncVar(sizeof(pthread_mutex_t), E_SYNC_MUTEX_LOCK, locktype);

      // Initialize the real mutex
      result = Real::pthread_mutex_init(real_mutex, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_MUTEX, mutex, real_mutex, sizeof(pthread_mutex_t));

    }
    else if(!current->disablecheck){
      // In the rollback phase, we will get the actual entry at first.
      resetSyncEntry(E_SYNCVAR_MUTEX, mutex, (void*)attr);  
      getSyncEventList(mutex, sizeof(pthread_mutex_t))->resetLockInfo(locktype);
    }
    //current->disablecheck is used for backtrace in rollback
    else {
      // current->disablecheck is true in re-execution phase
      result = Real::pthread_mutex_init(mutex, attr);
    }

    return result;
  }

  inline bool isInvalidSyncVar(void* realMutex) {
    return (((intptr_t)realMutex < xdefines::INTERNAL_HEAP_BASE) ||
                    ((intptr_t)realMutex >= xdefines::INTERNAL_HEAP_END)
                ? true
                : false);
  }

  int do_mutex_lock(void* mutex, thrSyncCmd synccmd) {
      
    int ret = 0;
  
    //FIXME Check current global status
checkAgain:
    checkStopStatus();
    // if there is no enough space, just invoke a new epoch
    if(!_sync.checkSyncSlots()){
#ifdef DUMP_LOG_TODISK
      disableCheck();
      dumpLogtoDisk(current->fp);
      enableCheck();
#else
      invokeCommit();
#endif
    }

    /* 
    // Check whether we should stop or not?   
    if(current->lockcounter == 0){  
      checkStopStatus();
      // if sync event list does not have enough slots, invoke a new epoch
      // now we use a slots buffer to avoid there is no slots between locks. e.g. cond_wait, nested lock 
      if(!_sync.checkSyncSlots() && !global_isStopStatus()){
        invokeCommit();
      }
    }
    */
    
    //PRINT("Thread %d mutex_lock at mutex %p\n", current->index, mutex);
    SyncEventList* list = NULL;
    pthread_mutex_t* realMutex = NULL;
    realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
    if(isInvalidSyncVar(realMutex)) {
      mutex_init((pthread_mutex_t*)mutex, NULL);
      realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
    }
      
    assert(realMutex != NULL);
    list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
    PRINF("Thread %d mutex_lock at mutex %p\n", current->index, mutex);

    if(!global_isRollback()) {

      switch(synccmd) {
        case E_SYNC_MUTEX_LOCK:
          ret = Real::pthread_mutex_lock(realMutex);
          break;

        case E_SYNC_MUTEX_TRY_LOCK:
          ret = Real::pthread_mutex_trylock(realMutex);
          break;

        default:
          break;
      }
      // double check stop status
      if(global_isStopStatus()) {
        Real::pthread_mutex_unlock(realMutex);
        goto checkAgain;
      }

      // Only update the lock when the acquisition is successful
      if(ret == 0) { 
#ifdef GET_CHARECTERISTICS
        __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
#endif
        current->lockcounter++; 
      }

      if(!current->disablecheck) {
        PRINF("Thread %d recording: mutex_lock at mutex %p realMutex %p list %p\n", current->index, mutex, realMutex, list);
        // Record this event
        list->recordSyncEvent(mutex, synccmd, ret);
      }
    } 
    else if (!current->disablecheck) {
      // PRINF("synceventlist get mutex at %p list %p\n", mutex, list);
      PRINF("REPLAY: Thread %d: mutex_lock at mutex %p list %p.\n", current->index, mutex, list);
      PRINF("Thread %d: in mutex_lock at mutex %p realMutex %p list %p\n", current->index, mutex, realMutex, list);
      assert(list != NULL);

      /* Peek the synchronization event (first event in the thread), it will confirm the following things
       1. Whether this event is expected event? If it is not, maybe it is caused by
          a race condition. Maybe we should restart the rollback or just simply reported this problem.
       2. Whether the first event is on the pending list, which means it is the thread's turn? 
          If yes, then the current signal thread should increment its semaphore.
       3. Whether the mutex_lock is successful or not? If it is not successful, 
          no need to wait for the semaphore since there is no actual lock happens.
      */ 
      if(_sync.peekSyncEvent(list, &ret) != true) {
        xrun::getInstance().rollbackDuringReexecution();  
      }
      PRINF("REPLAY: After peek: Thread %d: mutex_lock at mutex %p list %p ret %d.\n", current->index, mutex, list, ret);
      if(ret == 0) {
        waitSignal(list);
        current->lockcounter++; 
      }

      PRINF("REPLAY: After peekandwait: thread %d : mutex_lock at mutex %p list %p ret %d.\n", current->index, mutex, list, ret);
      //PRINF("mutex_lock at mutex %p list %p done!\n", mutex, list);
      _sync.advanceThreadSyncList();
    }

    if(ret == 0) {
      list->updateOwner(E_SYNC_MUTEX_LOCK, current);
    }

    return ret;
  }

  int mutex_lock(pthread_mutex_t* mutex) { 
    if (current->disablecheck) {
      // Turning the mutex_lock into noop in case of current->disablecheck is true.
      return 0;
    }
    else {
      return do_mutex_lock(mutex, E_SYNC_MUTEX_LOCK); 
    }
  }

  int mutex_trylock(pthread_mutex_t* mutex) { 
    if (current->disablecheck) {
      // Turning the mutex_lock into noop in case of current->disablecheck is true.
      // return Real::pthread_mutex_trylock((pthread_mutex_t *)mutex);
      return 0;
    }
    else {
      return do_mutex_lock(mutex, E_SYNC_MUTEX_TRY_LOCK);
    } 
  }

  int mutex_unlock(pthread_mutex_t* mutex) {
    int ret = 0;
    pthread_mutex_t* realMutex = NULL;

    SyncEventList* list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
    list->updateOwner(E_SYNC_MUTEX_UNLOCK, current);

    //PRINT("Thread %d mutex_unlock at mutex %p\n", current->index, mutex);
    if(!global_isRollback()) {
      //PRINT("line %d: Thread %d mutex_unlock at mutex %p\n", __LINE__, current->index, mutex);
      realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
      ret = Real::pthread_mutex_unlock(realMutex);

      if(current->lockcounter>0) current->lockcounter--; //Hongyu 
      PRINF("Thread %d mutex_unlock at mutex %p, realMutex %p\n", current->index, mutex, realMutex);
    }
    else if(!current->disablecheck) {
      PRDBG("thread %d mutex_unlock at mutex %p list %p\n", current->index, mutex, list);

      if(current->lockcounter>0) {
        signalNextThread(list);
      }else{
        PRINT("[WARNING] thread %d mutex_unlock at mutex %p list %p\n", current->index, mutex, list);
        abort();
      }

      /*  
      struct syncEvent* nextEvent = list->advanceSyncEvent();
      if(nextEvent) {
        _sync.signalNextThread(nextEvent);
      }
      */
      if(current->lockcounter>0) current->lockcounter--; //Hongyu 
      PRINF("thread %d mutex_unlock at mutex %p list %p\n", current->index, mutex, list);
    } 
    else {
      // Turning the mutex_unlock into noop in case of current->disablecheck is true.
      //Real::pthread_mutex_unlock(mutex);
      ret = 0;
    }

    return ret;
  }

  // Add this event into the destory list.
  int mutex_destroy(pthread_mutex_t* mutex) {
    if(!global_isRollback()) {
      deferSync((void*)mutex, E_SYNCVAR_MUTEX);
    }
    else if(current->disablecheck){
      Real::pthread_mutex_destroy(mutex); 
    }
  
    return 0;
  }

  int rwlock_init(pthread_rwlock_t *__restrict rwlock, const pthread_rwlockattr_t *__restrict attr) {
    int result=0;
    if(!global_isRollback()) {
      // Allocate a read/write lock
      pthread_rwlock_t* real_rwlock =
          (pthread_rwlock_t*)allocRealSyncVar(sizeof(pthread_rwlock_t), E_SYNC_RWLOCK_LOCK, -1);

      // Initialize the real mutex
      result = Real::pthread_rwlock_init(real_rwlock, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_RWLOCK, rwlock, real_rwlock, sizeof(pthread_rwlock_t));
    }
    else if(!current->disablecheck){
      // In the rollback phase, we will get the actual entry at first.
      resetSyncEntry(E_SYNCVAR_RWLOCK, rwlock, (void*)attr);  
      getSyncEventList(rwlock, sizeof(pthread_rwlock_t))->resetLockInfo(-1);
    }
    //current->disablecheck is used for backtrace in rollback
    else {
      // current->disablecheck is true in re-execution phase
      // FIXME how this happens? 
      result = Real::pthread_rwlock_init(rwlock, attr);
    }

    return result;
  }
  
  int rwlock_rdlock(pthread_rwlock_t *rwlock) { 
    if (current->disablecheck) {
      // this is impossible
      return 0; 
    }
    else {
      return rwlock_lock(rwlock, E_SYNC_RWLOCK_RDLOCK); 
    }
  }
  //int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

  int rwlock_wrlock(pthread_rwlock_t *rwlock) {
    if (current->disablecheck) {
      // this is impossible
      return 0; 
    }
    else {
      return rwlock_lock(rwlock, E_SYNC_RWLOCK_WRLOCK); 
    }
  }
  //int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
  
  int rwlock_lock(pthread_rwlock_t *rwlock, thrSyncCmd synccmd) {
    
    int ret = 0;
  
    // check whether we should stop or not ?
checkAgain:
    checkStopStatus();
    // if there is no enough space, just invoke a new epoch
    if(!_sync.checkSyncSlots()){
#ifdef DUMP_LOG_TODISK
      disableCheck();
      dumpLogtoDisk(current->fp);
      enableCheck();
#else
      invokeCommit();
#endif
    }

    // TODO stop here 

    //PRINT("Thread %d rwlock_lock at %p\n", current->index, rwlock);
    SyncEventList* list = NULL;
    pthread_rwlock_t *realRWLock = NULL;
    realRWLock = (pthread_rwlock_t *)getSyncEntry(rwlock);
    if(isInvalidSyncVar(realRWLock)) {
      rwlock_init(rwlock, NULL);
      realRWLock = (pthread_rwlock_t *)getSyncEntry(rwlock);
    }
      
    assert(realRWLock != NULL);
    list = getSyncEventList(rwlock, sizeof(pthread_rwlock_t));
    PRINF("Thread %d rwlock_lock at %p\n", current->index, rwlock);

    if(!global_isRollback()) {

      switch(synccmd) {
        case E_SYNC_RWLOCK_WRLOCK:
          ret = Real::pthread_rwlock_wrlock(realRWLock);
          break;

        case E_SYNC_RWLOCK_RDLOCK:
          ret = Real::pthread_rwlock_rdlock(realRWLock);
          break;

        default:
          break;
      }
      // double check stop status
      if(global_isStopStatus()) {
        Real::pthread_rwlock_unlock(realRWLock);
        goto checkAgain;
      }

      // Only update the lock when the acquisition is successful
      if(ret == 0) { 
#ifdef GET_CHARECTERISTICS
        __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
#endif
        current->lockcounter++; 
      }

      if(!current->disablecheck) {
        PRINF("Thread %d recording: rwlock_lock at %p realMutex %p list %p\n", current->index, rwlock, realRWLock, list);
        // Record this event
        list->recordSyncEvent(realRWLock, synccmd, ret);
      }
    } 
    else if (!current->disablecheck) {
      PRINF("REPLAY: Thread %d: rwlock_lock at mutex %p list %p.\n", current->index, rwlock, list);
      PRINF("Thread %d: in rwlock_lock at %p realRWLock %p list %p\n", current->index, rwlock, realRWLock, list);
      assert(list != NULL);

      /* Peek the synchronization event (first event in the thread), Whether this event is expected event? 
       * If it is not, maybe it is caused by a race condition. 
       * Maybe we should restart the rollback or just simply reported this problem.
      */ 
      if(_sync.peekSyncEvent(list, &ret) != true) {
        xrun::getInstance().rollbackDuringReexecution();  
      }

      /* Whether the first event is on the pending list, which means it is the thread's turn? 
       * If yes, then the current signal thread should increment its semaphore.
       * If the next event is also shared, release lock 
      */
      PRINF("REPLAY: After peek: Thread %d: rwlock_lock at %p list %p ret %d.\n", current->index, rwlock, list, ret);
      if(ret == 0) {
        waitExclusiveSignal(list);
        current->lockcounter++; 
      }

      PRINF("REPLAY: After peekandwait: thread %d : rwlock_lock at %p list %p ret %d.\n", current->index, rwlock, list, ret);
      //PRINF("mutex_lock at mutex %p list %p done!\n", mutex, list);
      _sync.advanceThreadSyncList();
    }
    
    /* Whether the mutex_lock is successful or not? 
     * If so, update counter and lock type for per-variable list,
     * in order that following thread can determine whether it can get the lock 
     * If it is not successful, no need to wait
    */
    if(ret == 0) {
      list->updateOwner(synccmd, current);
    }

    return ret;
  }

  int rwlock_unlock(pthread_rwlock_t *rwlock) {
    int ret = 0;
    pthread_rwlock_t* realLock = NULL;

    if(!global_isRollback()) {
      realLock = (pthread_rwlock_t*)getSyncEntry(rwlock);
      ret = Real::pthread_rwlock_unlock(realLock);

      if(current->lockcounter>0) current->lockcounter--; 
      PRINF("Thread %d rwlock_unlock at rwlock %p, real lock %p\n", current->index, rwlock, realLock);
    }
    else if(!current->disablecheck) {
      SyncEventList* list = getSyncEventList(rwlock, sizeof(pthread_rwlock_t));
      PRDBG("Thread %d rwlock_unlock at rwlock %p, list %p\n", current->index, rwlock, list);

      if(current->lockcounter>0) {
        // only the last read thread or write thread signal next
        signalNextExclusiveThread(list);
      }else{
        PRINT("[WARNING] thread %d rwlock_unlock at rwlock %p, real lock %p\n", current->index, rwlock, list);
        abort();
      }

      if(current->lockcounter>0) current->lockcounter--;
      PRINF("Thread %d rwlock_unlock at rwlock %p, list %p\n", current->index, rwlock, list);
    } 
    else {
      // Turning the unlock into noop in case of current->disablecheck is true.
      ret = 0;
    }

    return ret;
  }

  int rwlock_destroy(pthread_rwlock_t *rwlock) {
    if(!global_isRollback()) {
      //deferSync((void*)rwlock, E_SYNCVAR_RWLOCK);
    }
    else if(current->disablecheck){
      Real::pthread_rwlock_destroy(rwlock); 
    }
    return 0;
  }

  ///// conditional variable functions.
  int cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr) {
    if(!global_isRollback()) {
      // Allocate a mutex
      pthread_cond_t* real_cond =
          (pthread_cond_t*)allocRealSyncVar(sizeof(pthread_cond_t), E_SYNC_COND, 0);

      // Initialize the real mutex
      int result = Real::pthread_cond_init(real_cond, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_COND, cond, real_cond, sizeof(pthread_cond_t));
      PRINF("cond_init for thread %d. cond %p realCond %p\n", current->index, cond, real_cond);

      return result;
    }
    else {
      resetSyncEntry(E_SYNCVAR_COND, cond, (void*)attr);  
      return 0;
    }
  }

  // Add this into destoyed list.
  void cond_destroy(pthread_cond_t* cond) { 
    if(!global_isRollback()) {
      deferSync((void*)cond, E_SYNCVAR_COND); 
    }
  }

  int cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    return cond_wait_core(cond, mutex, NULL);
  }

  // Cond_timedwait: since we usually get the mutex before this. So there is
  // no need to check mutex any more.
  int cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec * abstime) {
    return cond_wait_core(cond, mutex, abstime);
  }

  // Condwait: since we usually get the mutex before this. So there is
  // no need to check mutex any more.
  int cond_wait_core(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec * abstime) {
    int ret;
    pthread_mutex_t* realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
    pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);
    assert(realMutex != NULL);

    // Check whether the cond is available.
    if(isInvalidSyncVar(realCond)) {
      cond_init((pthread_cond_t*)cond, NULL);
      realCond = (pthread_cond_t*)getSyncEntry(cond);
    }

    SyncEventList* list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
    bool toCheckpoint = false;
    bool hasCheckpoint = false;

#ifdef GET_CHARECTERISTICS
    if(!global_isRollback()) {
      __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
    }
#endif

wait_again:
    // We may have to save my context. Typically, it only happens after a cordinator met
    // an epoch and notify the current thread to checkpoint. 
    // The thread has to be waken up in order to keep a consistent state across all threads.
    // Otherwise, the thread waiting on conditional variables may rollback to the previous epoch
    if(toCheckpoint) {
    //  assert(abstime == NULL);
      toCheckpoint = false;
      hasCheckpoint = true;
      //saveContextWithoutWaiting();
      
      Real::pthread_mutex_unlock(realMutex);
      saveContextSync();

      if(!global_isRollback()) {
        Real::pthread_mutex_lock(realMutex);
      }
    }
      
    PRINF("cond_wait_core for thread %d. cond %p mutex %p\n", current->index, cond, mutex);

    if(!global_isRollback()) {
      PRINF("cond_wait for thread %d. cond %p mutex %p\n", current->index, cond, mutex);
      assert(realCond != NULL);

      if(!hasCheckpoint){
        list->updateOwner(E_SYNC_MUTEX_UNLOCK, current);
      }
      // Check whether it is a normal condwait or not.
      if(abstime == NULL) {
        // A normal condwait
        setThreadWaiting(realCond, E_THREAD_COND_WAITING);
     
        ret = Real::pthread_cond_wait(realCond, realMutex);
      }
      else {
        // The cond_timedwait. 
        setThreadWaiting(realCond, E_THREAD_COND_TIMED_WAITING); 
        ret = Real::pthread_cond_timedwait(realCond, realMutex, abstime);
      }

      // The checking will have several possible actions: rollback (no return), checkpoint, or proceed. 
      // cond_wait would release and acquire lock here, do 
      checkAtThreadWakenup(realMutex, &toCheckpoint);
    
      if(toCheckpoint) 
        goto wait_again; 
      
      list->updateOwner(E_SYNC_MUTEX_LOCK, current);
      // Record the waking up of conditional variable
      syncEvent* event = list->recordSyncEvent(mutex, E_SYNC_COND_WAIT, ret);
      // the lock should be recorded, so that it can be re-acquired in new epoch
      saveHoldingLocks(event);
      PRINF("Thread %d at cond_wait_core. cond %p realmutex %p list %p. Now wakenup\n", current->index, cond, realMutex, list); 
    } 
    else {
      // If it is correct, then get the actual list. 
      //list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
      PRINF("Thread %d: condwait at mutex %p cond %p: advanceSyncEvent on list %p\n", current->index, mutex, cond, list);

      // FIXME 
      // 1. if we rollback from here, thread would signal event first. 
      // 2. thread would peek sync event from list, it only checks whether it's from the same list. We need check whether it is the coming event
      if(!hasCheckpoint){
        list->updateOwner(E_SYNC_MUTEX_UNLOCK, current);
        // Before the condWait, we will have to release the current lock.     
        signalNextThread(list);
      }else{
        // if rollback here, assume it has lock again 
        if(current->lockcounter == 0) current->lockcounter++; 
      }

      /*
       * It is possible that we will get a wrong call during re-execution,
       * thus we should check whether this call is a correct event or not. 
       * If not, report immediately!!!!
       */
      PRINF("Thread %d: condwait at mutex %p cond %p: advanceSyncEvent on list %p\n", current->index, mutex, cond, list);
      // Check the diverge by comparing against the next event of the current thread.
      if(_sync.checkSyncEvent(mutex, E_SYNC_COND_WAIT) != true) {
        // Rollback immediately
        xrun::getInstance().rollbackDuringReexecution();
      } 

      //PRINF("Thread %d: condwait at mutex %p: advanceSyncEvent on list %p signal nextEvent %p\n", current->index, mutex, list, nextEvent);
      // Then we will have a mutex_lock after wakenup.
      if(_sync.peekSyncEvent(list, &ret) != true) {
        xrun::getInstance().rollbackDuringReexecution();  
      }

      if(global_rbNumber() > 1) { /*  PRINT("thread %d sleep 2s\n", current->index); */
        // This is to avoid race conditions related to conditional variables, e.g. bodytrack. 
        //sleep(2); 
      }
      PRINF("Thread %d: condwait at mutex %p: advanceSyncEvent on list %p. Return value is %d\n", current->index, mutex, list, ret);
      
      // Now waiting for the lock
      waitSignal(list);

      list->updateOwner(E_SYNC_MUTEX_LOCK, current);

      PRINF("thread %d : condwait at mutex %p: advanceSyncEvent on list %p. Wakenup\n", current->index, mutex, list);
      _sync.advanceThreadSyncList();
    }

    return ret;
  }
 
  //TODO The asymmetric race in Bodytrack happens between subroutine
  //Condition::Wait, where variable nWakeupTickets is read and written
  //inside a critical section, and subroutine Condition::NotifyOne,
  //where it is written outside any critical section.
  //If cond_broadcast or cond_signal are in critical section, doubletake can replay well.
  //Otherwise, it can not. We should consider the "happen-before" relationship between
  //cond_wait and cond_broadcast, cond_signal. 
  //
  //threads/Condition.cpp
  //NotifyOne()                 Wait()
  //-----------                                 ----------------
  //slack = nWaiting - nWakeupTickets;      nWaiting++;
  //if(slack > 0) { 
  //  nWakeupTickets = nWaiting;        //nWakeupTickets protects against spurious wakeups
  //  rv = pthread_cond_broadcast(&c);    while(nWakeupTickets == 0) {
  //                        rv = pthread_cond_wait(&c, &(M->m));
  //                      }
  //-------------------------------------------------------------------
  //AsyncIO.cpp
  //T1                  T2
  //while(mImageBuffer.size() == 0){  mDataLock.Lock(); 
  //  mLock2.Lock();          mImageBuffer.push_back(images);
  //  mCondEmpty.Wait();        mFGBuffer.push_back(FGImages);
  //  mLock2.Unlock();        mDataLock.Unlock();
  //}                 mCondEmpty.NotifyOne();
  //--------------------------------------------------------------------
  int cond_broadcast(pthread_cond_t* cond) { 
    pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);
    if(isInvalidSyncVar(realCond)) {
      cond_init((pthread_cond_t*)cond, NULL);
      realCond = (pthread_cond_t*)getSyncEntry(cond);
    }

    PRINF("Thread %d broadcast on cond %p\n", current->index, cond);
    if(!global_isRollback()) {
      return Real::pthread_cond_broadcast(realCond); 
    }
    else {
      // During the rollback pahse, this should be NULL. 
      assert(realCond != NULL);
      return 0;
    }
  }

  int cond_signal(pthread_cond_t* cond) { 
    pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);
    if(isInvalidSyncVar(realCond)) {
      cond_init((pthread_cond_t*)cond, NULL);
      realCond = (pthread_cond_t*)getSyncEntry(cond);
    }

    PRINF("Thread %d: singal the one on the cond %p\n", current->index, cond);
    if(!global_isRollback()) {
      return Real::pthread_cond_signal(realCond); 
    }
    else {
      // During the rollback pahse, this should be NULL. 
      assert(realCond != NULL);
      return 0;
    }
  }

  // Barrier initialization. Typically, barrier_init is to be called initially  
  int barrier_init(pthread_barrier_t* barrier, const pthread_barrierattr_t* attr, unsigned int count) {
    int result = 0;
    
    if(!global_isRollback()) {
      // Allocate the mutex and cond related to this barrier.
      pthread_cond_t * cond;
      pthread_mutex_t * mutex;
      size_t totalSize = sizeof(pthread_cond_t) + sizeof(pthread_mutex_t) + sizeof(BarrierInfo);
      void * ptr = allocRealSyncVar(totalSize, E_SYNC_BARRIER, 0);
      BarrierInfo * info;

      // Set the real barrier
      if(setSyncEntry(E_SYNCVAR_BARRIER, barrier, ptr, totalSize)) {
        PRINF("can't set synchronization entry??");
        assert(0);
      }
      PRINF("barrier_init barrier %p pointing to %p (while next %p) with count %d\n", barrier, ptr, *((void **)((intptr_t)barrier + sizeof(void *))), count);   

      // Initialize it.
      getBarrierInfo(barrier, &mutex, &cond, &info);

      // Actual initialization
      Real::pthread_cond_init(cond, NULL);
      Real::pthread_mutex_init(mutex, NULL);

      info->maxThreads = count;
      info->isArrivalPhase = true;
      info->waitingThreads = 0;
    }
    else {
      resetSyncEntry(E_SYNCVAR_BARRIER, barrier, 0);  
    }
    return result;
  }

  int barrier_destroy(pthread_barrier_t* barrier) {
    if(!global_isRollback()) {
    //  PRINF("barrier_destroy on barrier %p next %p\n", barrier, *((void **)((intptr_t)barrier + sizeof(void *))));
      deferSync((void*)barrier, E_SYNCVAR_BARRIER);
    //  PRINF("barrier_destroy on barrier %p done\n", barrier);
    }
    return 0;
  }

  ///// mutex functions
  void getBarrierInfo(pthread_barrier_t * barrier, pthread_mutex_t ** mutex, pthread_cond_t ** cond, BarrierInfo ** info) {
    // Get the real synchronization entry     
    void * ptr = getSyncEntry(barrier);
    assert(ptr != NULL);

    *mutex = (pthread_mutex_t *)(ptr);
    *cond = (pthread_cond_t *)((unsigned long)ptr+ sizeof(pthread_mutex_t));
    *info = (BarrierInfo *)((intptr_t)(*cond) + sizeof(pthread_cond_t));
  }

  // Add the barrier support. We have to re-design the barrier since 
  // it is impossible to wake up a thread that is waiting on a barrier.
  // Thus, it is impossible to rollback such a thread safely.
  int barrier_wait(pthread_barrier_t* barrier) {
    int ret = 0;
    pthread_mutex_t * mutex;
    pthread_cond_t * cond;
    BarrierInfo * info;

#ifdef GET_CHARECTERISTICS
   __atomic_add_fetch(&count_syncevent, 1, __ATOMIC_RELAXED);
#endif
    // Check whether we should rollback or not?
    checkStopStatus();

    // Get the actual barrier information
    getBarrierInfo(barrier, &mutex, &cond, &info);

    assert(mutex != NULL);

    Real::pthread_mutex_lock(mutex);

    // Check whether it is at the arrival phase
    // Someone else is exiting, which is a very short period 
    // and the current thread will never be stucked here.
    while(info->isArrivalPhase != true) {
      Real::pthread_cond_wait(cond, mutex);
    }

    // Now in an arrival phase, proceed with barrier synchronization
    info->waitingThreads++;

    if(info->waitingThreads >= info->maxThreads) {
      info->isArrivalPhase = false;

      //treat the last thread as serial thread
      ret = PTHREAD_BARRIER_SERIAL_THREAD;

      // Waking up all threads that are waiting on this barrier now.
      Real::pthread_cond_broadcast(cond);
    } 
    else {
      assert(info->isArrivalPhase == true);
      bool toCheckpoint = false;
barrier_waitagain:
      if(toCheckpoint) {
        Real::pthread_mutex_unlock(mutex);
        toCheckpoint = false;
        saveContext();
        Real::pthread_mutex_lock(mutex);
      }

      // Now we are waiting on the barrier.
      while(info->isArrivalPhase == true) {
        setThreadWaiting(cond, E_THREAD_BARRIER_WAITING);
        
        // Actual wait on the conditional variable.
        Real::pthread_cond_wait(cond, mutex);

        // For normal wakenup, isArrivalPhase will be set to false and force the 
        // current thread to leave this barrier.
        // Otherwise, if the thread is notified to checkpoint, then 
        // isArrivalPhase must be ''true'' and it will enter the next loop
        checkAtThreadWakenup(mutex, &toCheckpoint);
        if(toCheckpoint){
          goto barrier_waitagain;
        }
      }
    }

    info->waitingThreads--; 

    // When all threads leave the barrier, entering into the new arrival phase.  
    if(info->waitingThreads == 0) {
      info->isArrivalPhase = true;

      // Wakeup all threads that are waiting on the arrival fence
      Real::pthread_cond_broadcast(cond);
    }

    Real::pthread_mutex_unlock(mutex);

    if (!current->disablecheck){
      if(!global_isRollback()) {
        _sync.recordBarrierVal(ret, E_SYNC_BARRIER); 
      }else {
        if(_sync.peekSyncEvent(NULL, &ret) != true) {
          xrun::getInstance().rollbackDuringReexecution();
        }
        _sync.advanceThreadSyncList();
      }
    }

    return ret;
  }

  // Save the given signal handler.
  void saveSpecifiedContext(ucontext_t* context) {
    current->bakContext.saveSpecifiedContext(context); //Hongyu
  }

  // Return actual thread index
  int getIndex() { return current->index; }

  // Preparing the rollback.
  void prepareRollback();
  void prepareRollbackAlivethreads();
//  void destroyAllSemaphores();
//  void initThreadSemaphore(thread_t* thread);
//  void resetThreadSemaphore(thread_t* thread);
//  void destroyThreadSemaphore(thread_t* thread);
  void wakeupOldThreadsDuringRollback();
  static void wakeupThreadsAtEpochBegin(int threadnum);

  static void epochBegin(thread_t * thread);

  // Rollback the current thread
  static void rollbackCurrent();
  void checkRollbackCurrent();

  // It is called when a thread has to rollback.
  // Thus, it will replace the current context (of signal handler)
  // with the old context.
  void rollbackInsideSignalHandler(ucontext_t* context) {
    xcontext::rollbackInsideSignalHandler(context, &current->bakContext);
  }

  inline static pthread_t thread_self() { return Real::pthread_self(); }

  inline static void saveContext() {
    // all threads would save context, when entering into a new epoch,
    // this place should be a good place to get locks again
    xthread::getInstance().resetHoldingLocks();

    saveContextSync();
  }

  inline static void saveContextSync() {
    global_waitfence();
    saveContextWithoutWaiting();
  }

  inline static void saveContextWithoutWaiting() {
    current->bakContext.saveContext();
  }

  inline static void restoreContext() {
    PRINF("thread %d restore context now\n", current->index);
    //xcontext::restoreContext(&current->oldContext, &current->newContext); //Hongyu
    xcontext::restoreContext(&current->bakContext);
  }

  // We will rollback based on old context. We will leave newContext intactly
  inline static void rollbackContext() { assert(0); }

  // Run those deferred synchronization.
  inline static void runDeferredSyncs() { threadinfo::getInstance().runDeferredSyncs(); }
  
  bool checkSkipStatusAndActions();
  bool checkSkipStatus();
  void checkStopStatus();
  void handleSignal();
  void stopCurrentThread();
  void stopCurrentThreadWithoutCheckpoint();
  void releaseHoldingLocks(pthread_mutex_t* mutex=NULL);
  void resetHoldingLocks();

  inline static void enableCheck() {
    current->internalheap = false;
    current->disablecheck = false;
     //PRINT("Enable check for current %p disablecheck %d\n", current, current->disablecheck);
  }

  inline static bool isCheckDisabled() { return current->disablecheck; }

  inline static void disableCheck() {
    current->internalheap = true;
    current->disablecheck = true;
     //PRINT("Disable check for current %p disablecheck %d\n", current, current->disablecheck);
  }

  inline static pid_t gettid() { return syscall(SYS_gettid); }

  static void invokeCommit();
#ifdef DETECT_USAGE_AFTER_FREE
  bool addQuarantineList(void* ptr, size_t sz);
#endif

#ifdef DUMP_LOG_TODISK
  void dumpLogtoDisk(FILE* fp){
    current->syncevents.dumpToDisk(fp);
  }
#endif

private:

  void saveHoldingLocks(syncEvent* mutex);

  inline void* getSyncEntry(void* entry) {
    void** ptr = (void**)entry;
    return (*ptr);
  }

  inline void  resetSyncEntry(syncVariableType type, void * nominal, void* attr) {
      // Geting the starting address of real synchronization variable.
      void * real = _sync.retrieveRealSyncEntry(type, nominal);
      if(real != NULL) {
        *((void **)nominal) = real;
      }
      else if(type == E_SYNCVAR_MUTEX) {
        // Somehow, replay phase may call different lock, for example, backtrace.
        // Allocate an real entry for that.
        // Allocate a mutex
        int locktype = -1;
        if(attr) pthread_mutexattr_gettype((pthread_mutexattr_t*)attr, &locktype);
        pthread_mutex_t* real_mutex =
          (pthread_mutex_t*)allocRealSyncVar(sizeof(pthread_mutex_t), E_SYNC_MUTEX_LOCK, locktype);

        // Initialize the real mutex
        Real::pthread_mutex_init(real_mutex, (pthread_mutexattr_t*)attr);

        // If we can't setup this entry, that means that this variable has been initialized.
        setSyncEntry(E_SYNCVAR_MUTEX, nominal, real_mutex, sizeof(pthread_mutex_t));
      }
  }

  inline SyncEventList* getSyncEventList(void* ptr, size_t size) {
    void** entry = (void**)ptr;
    //    PRINF("ptr %p *entry is %p, size %ld\n", ptr, *entry, size);
    return (SyncEventList*)((intptr_t)(*entry) + size);
  }

  inline int setSyncEntry(syncVariableType type, void* syncvar, void* realvar, size_t size) {
    int ret = -1;
    unsigned long* target = (unsigned long*)syncvar;
    unsigned long expected = *(unsigned long*)target;

    // if(!__sync_bool_compare_and_swap(target, expected, (unsigned long)realvar)) {
    if(!__atomic_compare_exchange_n(target, &expected, (unsigned long)realvar, false,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
      deallocRealSyncVar(realvar);
    } else {
      ret = 0;
      // It is always safe to add this list into corresponding map since
      // only those passed the CAS instruction can call this.
      // Thus, it is not existing two lists corresponding to the same synchronization variable.
      SyncEventList* list = getSyncEventList(syncvar, size);

      // Adding this entry to global synchronization map
      void * syncentry = _sync.recordSyncVar(type, (void*)syncvar, realvar, list);
      *((void **)((intptr_t)syncvar + sizeof(void *))) = syncentry;
      //if(type == E_SYNCVAR_BARRIER) {
      //  PRINF("Barrier setSyncEntry: syncvar %p next %p syncentry %p\n", syncvar,  *((void **)((intptr_t)syncvar + sizeof(void *))), syncentry);
      //}
    }

    return ret;
  }

  inline void* allocRealSyncVar(int size, thrSyncCmd synccmd, int lockType) {
    // We allocate a synchorniation event list here and attach to this real
    // synchronziation variable so that they can be deleted in the same time.
    void* entry = ((void*)InternalHeap::getInstance().malloc(size + sizeof(SyncEventList)));
    assert(entry != NULL);
    void* ptr = (void*)((intptr_t)entry + size);

    // Using placement new here
    new (ptr) SyncEventList(entry, synccmd, lockType);
    return entry;
  }

  inline void deallocRealSyncVar(void* ptr) { InternalHeap::getInstance().free(ptr); }

  // Mark whether 
  static void markWaitingSignal() {
    lock_thread(current);
    current->status = E_THREAD_WAITING_REEXECUTION;
    unlock_thread(current);
  }

  static void unmarkWaitingSignal() {
    lock_thread(current);
    current->status = E_THREAD_RUNNING;
    unlock_thread(current);

    // We have to call xthread::getInstance() since it is a static function 
    xthread::getInstance().checkStopStatus();
  }

  // check whether first event of per-thread list is equal to first event of global list
  inline static bool checkFirstEvent(SyncEventList* list){
    bool ret = false;
    syncEvent* event = list->getCurrentSyncEvent();
    syncEvent* currentevent = current->syncevents.firstIterEntry();
    if(event == currentevent){
      ret = true;
    } else if(list->checkRecursiveLock(currentevent)) {
      ret = true;
    }
    return ret;
  }

  inline static bool checkSharedEvent(SyncEventList* list){
    bool ret = false;
    syncEvent* event = list->getCurrentSyncEvent();
    syncEvent* currentevent = current->syncevents.firstIterEntry();
    if(event == currentevent){
      ret = true;
    } else if(list->checkReadWriteLock(currentevent, currentevent->synccmd)) {
      list->advanceSyncEvent();
      ret = true;
    }
    return ret;
  }

  // Acquire the semaphore for myself.
  // If it is my turn, I should get the semaphore.
  //FIXME SEM 
  inline static void waitSignal(SyncEventList* list) {
    markWaitingSignal();
    list->lock_eventlist();
    while(!checkFirstEvent(list) && !global_isStopStatus()){
      PRINF("thread %d is waiting at list %p", current->index, list);
      list->wait_eventlist(&current->synccond);
    }
    list->unlock_eventlist();
    unmarkWaitingSignal();
  }
  
  // signal next thread
  inline static void signalNextThread(SyncEventList* list) {

    list->lock_eventlist();
    // get next event in global list so that we can get next thread
    syncEvent* event = list->getNextSyncEvent();
    if(event){
      thread_t* thread = (thread_t*)event->thread;
      PRINF("thread %d signal next thread %d at list %p", current->index, thread->index, event->eventlist);
      //move to next event
      list->advanceSyncEvent();
      list->signal_eventlist(&thread->synccond);
    }
    list->unlock_eventlist();
  }

  /*** ----- this is used by read/write lock ----- **/
  inline static void waitExclusiveSignal(SyncEventList* list) {
    markWaitingSignal();
    list->lock_eventlist();
    while(!checkFirstEvent(list) 
        && !checkSharedEvent(list) 
        && !global_isStopStatus()){
      PRINF("thread %d is waiting at list %p", current->index, list);
      list->wait_eventlist(&current->synccond);
    }
    // only record lock counter in replay for read/write lock
    syncEvent* currentevent = current->syncevents.firstIterEntry();
    list->updateOwner(currentevent->synccmd, current);
    // signal next in case that next is also shared lock
    syncEvent* event = list->getNextSyncEvent();
    if(event){
      thread_t* thread = (thread_t*)event->thread;
      list->signal_eventlist(&thread->synccond);
    }

    list->unlock_eventlist();
    unmarkWaitingSignal();
  }
  
  // signal next thread
  inline static void signalNextExclusiveThread(SyncEventList* list) {

    list->lock_eventlist();

    // if this is read and write lock, we should check whether we need to move next.
    // Only the last thread should signal next thread 
    if(list->checkReadWriteLock(NULL, E_SYNC_RWLOCK_UNLOCK)) {
      // get next event in global list so that we can get next thread
      syncEvent* event = list->getNextSyncEvent();
      if(event){
        thread_t* thread = (thread_t*)event->thread;
        PRINF("thread %d signal next thread %d at list %p", current->index, thread->index, event->eventlist);
        //move to next event
        list->advanceSyncEvent();
        list->signal_eventlist(&thread->synccond);
      }
    }
    list->unlock_eventlist();
  }
  /*** ----- this is used by read/write lock end ----- **/

  // Newly created thread should call this.
  inline static void threadRegister(bool isMainThread) {
    pid_t tid = gettid();
    void* privateTop;
    size_t stackSize = __max_stack_size;

    current->self = Real::pthread_self();

    // Initialize event pool for this thread.
    //listInit(&current->pendingSyncevents);

    // Lock the mutex for this thread.
    lock_thread(current);

    // Initialize corresponding cond and mutex.
    //listInit(&current->list);

    current->tid = tid;
    current->status = E_THREAD_RUNNING;
    current->isNewlySpawned = true;

    current->disablecheck = false;

    current->joiner = NULL;

#ifdef DUMP_LOG_TODISK
    char filename[MAX_FILENAME_LEN];
    PRINF("thread %d dump data to %s_%d_eventlist.log\n", current->index, program_invocation_name, tid);
    snprintf(filename, MAX_FILENAME_LEN, "%s_%d_eventlist.log", program_invocation_name, tid);
    current->fp = Real::fopen(filename, "w");
    setvbuf(current->fp, NULL, _IOFBF, 1024*1024);
    //current->fp = Real::fopen(filename, "ab+");
#endif

    // Initially, we should set to check system calls.
    enableCheck();

    // Initialize the localized synchronization sequence number.
    // pthread_t thread = current->self;
    pthread_t thread = pthread_self();

    if(isMainThread) {
      void* stackBottom;
      current->mainThread = true;

      // First, we must get the stack corresponding information.
      selfmap::getInstance().getStackInformation(&stackBottom, &privateTop);
    } else {
      /*
        Currently, the memory layout of a thread private area is like the following.
          ----------------------  Higher address
          |      TCB           |
          ---------------------- pd (pthread_self)
          |      TLS           |
          ----------------------
          |      Stacktop      |
          ---------------------- Lower address
      */
      current->mainThread = false;
      // Calculate the top of this page.
      privateTop = (void*)(((intptr_t)thread + xdefines::PageSize) & ~xdefines::PAGE_SIZE_MASK);
    }

    current->bakContext.setupStackInfo(privateTop, stackSize);
    //current->newContext.setupStackInfo(privateTop, stackSize); //Hongyu
    current->stackTop = privateTop;
    current->stackBottom = (void*)((intptr_t)privateTop - stackSize);
    
    // Save the context for non-main thread. Main thread already did this before
    if(!isMainThread) {
      saveContextWithoutWaiting();
    
      // Wakeup its parent since the parent is waiting for the finish of the registering
      signal_thread(current);
    }

    PRINF("THREAD%d (pthread_t %p) registered at %p, status %d wakeup %p. lock at %p\n",
          current->index, (void*)current->self, current, current->status, &current->cond,
          &current->mutex);

    unlock_thread(current); 
  }

  static bool isThreadDetached() { return current->isDetached; }

  /// @ internal function: allocation a thread index
  int allocThreadIndex() { return threadinfo::getInstance().allocThreadIndex(); }

  inline thread_t* getThreadInfo(int index) { return threadinfo::getInstance().getThreadInfo(index); }

  inline thread_t* getThreadByTid(pthread_t thread) {
    return threadmap::getInstance().getThreadInfo(thread);
  }

  // Actually calling both pdate both thread event list and sync event list.
  inline void updateSyncEvent(SyncEventList* list) {
    // Advance the thread eventlist
    _sync.advanceThreadSyncList();
    signalNextThread(list);
    /*  
    struct syncEvent* event = list->advanceSyncEvent();
    if(event) {
      // Actually, we will wakeup next thread on the event list.
      // Since cond_wait will call unlock at first.
      _sync.signalNextThread(event);
    }
    */
  }

  inline void insertAliveThread(thread_t* thread, pthread_t tid) {
    threadmap::getInstance().insertAliveThread(thread, tid);
  }

  static bool isStackVariable(void* ptr) {
    return (ptr >= current->stackBottom && ptr <= current->stackTop);
  }

  // Insert a synchronization variable into the global list, which
  // are reaped later in the beginning of next epoch.
  inline static bool deferSync(void* ptr, syncVariableType type) {
    if(type == E_SYNCVAR_THREAD) {
      return threadinfo::getInstance().deferSync(ptr, type);
    }
    else {
      xsync::SyncEntry * entry = (xsync::SyncEntry *)(*((void **)((intptr_t)ptr + sizeof(void *))));

      //if(type == E_SYNCVAR_BARRIER) {
      //  PRINF("Barrier before detroy ptr %p entry %p\n", ptr, entry);
      //}
      xsync::getInstance().deferSync(entry);
      return true;
    }
  }


  // @Global entry of all entry function.
  static void* startThread(void* arg) {
    void* result = NULL;
    current = (thread_t*)arg;

    // PRINF("thread %p self %p is starting now.\n", current, (void*)current->self);
    // Record some information before the thread moves on
    threadRegister(false);

    // Now current thread is safe to be interrupted.
    PRINF("Thread %d at %p self %p after thread register now.\n", current->index, current, (void*)current->self);

    // We actually get those parameter about new created thread
    // from the TLS storage.
    try{
      result = current->startRoutine(current->startArg);
      PRINF("Thread %d at %p has finished now.\n", current->index, current);
    }
    catch (int err){
      if(err != PTHREADEXIT_CODE){
        throw err;
      }
    }//Hongyu
 
    PRINF("Calling startRoutine %p on thread %d finished\n", current->startRoutine, current->index);
    // Insert dead list now so that the corresponding entry can be cleaned up if
    // there is no need to rollback.
    bool toCheckpoint = false;
DO_CHECKPOINT:
    if(toCheckpoint == true) {
        toCheckpoint = false;
        saveContext();
    }

    // Lock the mutex for this thread.
    lock_thread(current);

    current->result = result;
    current->status = E_THREAD_WAITFOR_REAPING;

    // Only check the joiner when the thread is not deatached.
    if(!isThreadDetached()) {
        // Check the state of joiner.
        if(current->joiner) {
            assert(current->joiner->status == E_THREAD_JOINING);
            PRINF("Waking up the joiner %p!!! status %d\n", (void*)current->joiner->self, current->joiner->status);
            PRINF("thread %d Waking up the joiner %p!!!\n", current->index, (void*)current->joiner);

            // Now we can wakeup the joiner.
            signal_thread_joiner(current);
        }
    }
    else {
      current->hasJoined = true;
      PRINF("Thread is detached!!!\n");
    }


    current->action = E_THREAD_ACTION_NONE;

    // At epoch ends, if no error is detected, then the thread
    // should set the action to E_THREAD_ACTION_EXIT, otherwise its action should
    // be set to E_THREAD_ROLLBACK
    while(current->action != E_THREAD_ACTION_EXIT) {
        
      wait_thread(current);
      
      // Release the lock since checkAtThreadWakenup will acquire the mutex of the current thread
      unlock_thread(current);

      PRINF("thread %d is wakenup, action %d\n", current->index, current->action);
      xthread::getInstance().checkAtThreadWakenup(NULL, &toCheckpoint);

      if(toCheckpoint)  {
        // We have to checkpoint the status before the waiting.
        // Since epochBegin will set up the status to E_THREAD_RUNNING, and 
        // we do not acquire the lock now, we will jump outside the critical section (DO_CHECKPOINT)
        // It should not cause any problem since every thread will be waken up one by one
        // If the parent thread already joined this thread, then this thread should not be required to perform the checkpoint
        // The thread should be set to E_THREAD_ACTION_EXIT in this case 
        goto DO_CHECKPOINT;
      }
      
      lock_thread(current);
    }
    
#ifdef DUMP_LOG_TODISK
    Real::fclose(current->fp);
#endif

    unlock_thread(current);
      
    // Actual exit now so that this thread can be joined.
    assert(current->action == E_THREAD_ACTION_EXIT);
    PRINF("THREAD%d (at %p) is wakenup and plan to exit now\n", current->index, current);
    
    return result;
  }

  // They are claimed in xthread.cpp since I don't
  // want to create an xthread.cpp
  xsync& _sync;
  SysRecord _sysrecord;
  SyncEventList * _spawningList;
};

#endif
