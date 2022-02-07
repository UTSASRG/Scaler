/*
 * @file   xthread.cpp
 * @brief  Handle Thread related information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xthread.hh"

#include "globalinfo.hh"
#include "internalsyncs.hh"
#include "list.hh"
#include "log.hh"
#include "quarantine.hh"
#include "recordentries.hh"
#include "threadinfo.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xrun.hh"
#include "syscalls.hh"
#include "xsync.hh"

// Global lock used when joining and exiting a thread.
// threadmap::threadHashMap threadmap::_xmap;
// list_t threadmap::_alivethreads;

// bool xthread::_isRollbackPhase;
// list_t xthread::_deadSyncVars;
__thread thread_t* current;
// threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;


char* getThreadBuffer() {
  int index = getThreadIndex();

  return threadinfo::getInstance().getThreadBuffer(index);
}

void xthread::invokeCommit() {
  xrun::getInstance().epochEnd(false);
	PRINF("invokeCommit after epochEnd\n");
  xrun::getInstance().epochBegin();
}

//check skip status 
bool xthread::checkSkipStatus(){
	return current->action == E_THREAD_ACTION_EPOCH_SKIP;
}

// Check whether it should skip operations of epochEnd
bool xthread::checkSkipStatusAndActions(){
	bool ret = false;

	lock_thread(current);

  //RELEASELOCK
  xthread::getInstance().releaseHoldingLocks();

  //skip operations
	if(current->action == E_THREAD_ACTION_EPOCH_SKIP){
		
		//stop here like other threads
		current->status = E_THREAD_STOPPED;
    
    /************************************************************
       We will be waken up only once, thus no need to check 
       1. No rollback: when the co-ordinator thread  is in epochBegin 
          but before performing the memory backup.
       2. Rollback: the co-ordinator thread is in rollback phase during wakeupOldThreadsDuringRollback
          or pthread_create(). 
    *************************************************************/ 
	  wait_thread();

	  current->status = E_THREAD_RUNNING;
		//check thread status, if rollback, just do it. 
		//if keep going, set it SKIP in order to be used in epochBegin
		if(current->action == E_THREAD_ACTION_ROLLBACK) {
		  unlock_thread(current);
			
      // Simply rollback the current thread. Never return here.
			rollbackCurrent();	
		}
		ret = true;
	}
	
  unlock_thread(current);
	
  return ret;
}

void xthread::handleSignal() {
  struct simpleSyscall* entry;
  // This is a checkpoint. 
  // Here should be best place to handle signal here.
  if(!global_isRollback()) {
    if(current->sig != 0) {
      lock_thread(current);
      entry = current->syslist.alloc();
      entry->syscall = E_SYS_MAX + current->sig;
      current->sig = 0;
      unlock_thread(current);

      // invoke signal handler
      int sig = entry->syscall - E_SYS_MAX;
      // we treat pthread_cancel as a SIGKILL signal 
      if(sig == SIGKILL) {
        throw PTHREADEXIT_CODE;
      }
    }
  } else {
    entry = current->syslist.firstIterEntry();
    // this is a signal
    if(entry->syscall > E_SYS_MAX) {
      // move to next system call
      current->syslist.advanceEntry();
      // invoke signal handler
      int sig = entry->syscall - E_SYS_MAX;
      // we treat pthread_cancel as a SIGKILL signal 
      if(sig == SIGKILL) {
        throw PTHREADEXIT_CODE;
      }
    }
  }
}

void xthread::checkStopStatus() {

  handleSignal();

  if(global_isStopStatus()) {
    stopCurrentThread();
  }	
}

// This will be stopped during both original execution or re-execution
void xthread::stopCurrentThread() {
	lock_thread(current);

  //RELEASELOCK
  xthread::getInstance().releaseHoldingLocks();
  
	current->status = E_THREAD_STOPPED; 

  // There are only two operations at epoch end: checkpoint, or rollback.
	while((current->action != E_THREAD_ACTION_CHECKPOINT) && (current->action != E_THREAD_ACTION_ROLLBACK) ) {
		wait_thread();
	}

  // Set the action to none. 
  current->status = E_THREAD_RUNNING;

	if(current->action == E_THREAD_ACTION_CHECKPOINT) {
	  current->action = E_THREAD_ACTION_NONE;

    // Wakeup the coordinator thread here
   // signal_thread_epochbegin(current);

	  unlock_thread(current);
		
		// Save the context
    // Good for cond_timedwait since it will wait again if condition is not true
		saveContext();	
	}
	else {
    // Rollback 
	  current->action = E_THREAD_ACTION_NONE;
	  unlock_thread(current);

		// Simply rollback the current thread.
		// We assume that the coordinator thread has done everything for me,
		// except the recovery of context and stack
		rollbackCurrent();	
	}
}

// This will be stopped during both original execution or re-execution
void xthread::stopCurrentThreadWithoutCheckpoint() {

	current->status = E_THREAD_STOPPED; 

  // There are only two operations at epoch end: checkpoint, or rollback.
	while((current->action != E_THREAD_ACTION_CHECKPOINT) && (current->action != E_THREAD_ACTION_ROLLBACK) ) {
		wait_thread();
	}
	
  // Set the action to none. 
  current->status = E_THREAD_RUNNING;

	if(current->action == E_THREAD_ACTION_ROLLBACK) {
    // Rollback 
	  current->action = E_THREAD_ACTION_NONE;
		unlock_thread(current);

		// Simply rollback the current thread.
		// We assume that the coordinator thread has done everything for me,
		// except the recovery of context and stack
		rollbackCurrent();	
	}
}

// This function is called by the co-ordinator thread, but not inside the current thread. 
void xthread::epochBegin(thread_t * thread) {
	
	// Now we should not have the pending synchronization events.	
	//listInit(&thread->pendingSyncevents);

	// Handle the quarantine list of memory.
#ifdef DETECT_USAGE_AFTER_FREE
  thread->qlist.restore();
#endif

	// Now we will cleanup recorded sychronizations	
  thread->syncevents.cleanup();

	// Cleanup all types of syscalls
	syscalls::getInstance().cleanupSyscalls(thread);	
}

void xthread::prepareRollbackAlivethreads() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();
    
    /*   
	  // Initialize the semaphore for this thread.
    //FIXME SEM we can delete this later 
	  if(global_rbNumber()==1){
		  initThreadSemaphore(thread);
	  }
    else{
      // If we are not the first time to rollback,
      // we donot have to recreate the semaphores but only resetting. 
		  resetThreadSemaphore(thread);
	  }
    */

    // Set the entry of each thread to the first synchronization event.
   	thread->syslist.prepareRollback();
   	thread->recordlist.prepareRollback();
	  thread->syncevents.prepareRollback();
    
    // reset lockcounter
    thread->lockcounter = thread->backup_lockcounter;
  }
}

//We can delete these functions
#if 0
/**
  destroy all semaphores:
*/
//FIXME SEM
void xthread::destroyAllSemaphores() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    // If we found the entry, remove this entry from the list.
    destroyThreadSemaphore(thread);
  }
}

// Initialize the semaphore for  specified thread
//FIXME SEM
void xthread::destroyThreadSemaphore(thread_t* thread) {
  semaphore* sema = &thread->sema;

  // We initialize the semaphore value to 0.
  sema->destroy();
}

// Initialize the semaphore for a specified thread
//FIXME SEM
void xthread::initThreadSemaphore(thread_t* thread) {
    semaphore* sema = &thread->sema;

    PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    //PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    // We initialize the semaphore value to 0.
    sema->init((unsigned long)thread->self, 1, 0);
}

// Initialize the semaphore for  specified thread
//FIXME SEM
void xthread::resetThreadSemaphore(thread_t* thread) {
    semaphore* sema = &thread->sema;
    PRINF("RESETSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    // We set the semaphore value to 0.
    sema->setVal(0);
}

#endif

void xthread::prepareRollback() {
  PRINF("calling syscalls::prepareRollback\n");
  PRINF("calling threadmap::prepareRollback\n");

	prepareRollbackAlivethreads();

  PRINF("calling xsync::prepareRollback\n");
  PRINF("before calling sync::prepareRollback\n");
  // Update the semaphores and synchronization entry
  _sync.prepareRollback();
	xsync::prepareEventListRollback(_spawningList);
	PRINF("after calling sync::prepareRollback\n");

	// Setting the phase to rollback
//	global_setRollback(); //Hongyu 
 
	// Now it is time to wake up those waiting threads
	// if they are not newly spawned in this epoch.
	wakeupOldThreadsDuringRollback();
}

/* Old waiting threads meant that those threads are created before the last epoch,
 * Thus, there is no need to wait for their parents' notifications.
 * Since other threads are actually in a stable waiting status, we don't have to acquire locks.
 */
void xthread::wakeupOldThreadsDuringRollback() {
	threadmap::aliveThreadIterator i;

	//FIXME may have the same problem as wakeupThreadsAtEpochBegin
	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();
 
		// Currently, we only care about those old threads since 
		// the parent will wakeup those newly spawned threads appropriately
		// Additonaly, there is no need to wake up the current thread 
		if(thread->isNewlySpawned != true && thread != current) {
			PRINF("wakeup thread %d at wakeupOldThreadsDuringRollback, status %d\n", thread->index, thread->status);

			thread->action = E_THREAD_ACTION_ROLLBACK;

      if(thread->status != E_THREAD_SIGWAITING ){
			  // If the thread is already at E_THREAD_WAITFOR_REAPING and it not a newly spawned thread,
			  // then we should wake this thread up immediately
			  signal_waiting_thread(thread);
      }else{
        //wake up sigwait thread using SIGPWR
        Real::pthread_kill(thread->self, SIGPWR);
      }
		}	
	}		
}

/* 
 * At epoch begin, we will wake up all threads that are not supposed to be at a waiting status
 * A thread is at the waiting status (based on the application) includes at least the following ones
    E_THREAD_SIGWAITING
		E_THREAD_COND_WAITING (but not E_THREAD_COND_TIMEWAITING, which is instablei)
		E_THREAD_BARRIER_WAITING
		E_THREAD_WAITFOR_JOINING
		E_THREAD_WAITFOR_REAPING
		E_THREAD_JOINING

    or 
    E_THREAD_ACTION_SKIP_EPOCH
	These threads actually perform the checking after the status has been changed. 
 */
void xthread::wakeupThreadsAtEpochBegin(int threadnum) {
	threadmap::aliveThreadIterator i;

  int condwaitnum = 0;
	// If two threads are waiting on the same condition variable, when we wake up one thread, 
	// we may wake up another thread, we can not control the scheduling. However, the action is not set for another thread.
	// Now we set action first, ane then wake up
	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
		thread_t* thread = i.getThread();
    
    //FIXME is it good to backup lock counter here
    thread->backup_lockcounter = thread->lockcounter;

		if(thread != current) {

      lock_thread(thread);
			// Now let all waiting threads to perform the checkpoint
			thread->action = E_THREAD_ACTION_CHECKPOINT;

      thrStatus curstat = thread->status;
			if( curstat == E_THREAD_COND_TIMED_WAITING
          || curstat == E_THREAD_COND_WAITING) {
        // set thread status to running
        thread->status = E_THREAD_RUNNING;
        condwaitnum++;
      }
      unlock_thread(thread);
		}
  }
  global_waitingThreads(condwaitnum);

  // wake up waiting thread. now the thread status is running
  // let cond_wait thread go first
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
		thread_t* thread = i.getThread();
		if(thread != current) {
      lock_thread(thread);
			if( thread->status == E_THREAD_RUNNING ) {
        signal_waiting_thread(thread);
      }
      unlock_thread(thread);
		}
  }

  int recovernum = 0;
  // make sure the thread is waiting again, in case the thread is not waiting, but lock is acquired by other thread.
  while(recovernum != condwaitnum){
    recovernum = 0;
    for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
      thread_t* thread = i.getThread();
      if(thread != current) {
        lock_thread(thread);
        thrStatus curstat = thread->status;
        if( curstat == E_THREAD_COND_TIMED_WAITING
            || curstat == E_THREAD_COND_WAITING) {
          recovernum++;
        }
        unlock_thread(thread);
      }
    }
  }

  global_waitingThreads(threadnum - condwaitnum);

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

		if(thread != current) {

      if(thread->status != E_THREAD_COND_TIMED_WAITING && thread->status != E_THREAD_COND_WAITING){
        lock_thread(thread);

        // Now let all waiting threads to perform the checkpoint
        //	thread->action = E_THREAD_ACTION_CHECKPOINT;

        // FIXME: how to handle sigwaiting
        if(thread->status != E_THREAD_SIGWAITING) {
          // Wakeup threads so that they can perform a checkpoint
          signal_waiting_thread(thread);

          // while(thread->action != E_THREAD_ACTION_NONE){
          //   wait_thread_epochbegin(thread);  
          // }

        }
        else{
          // use SIGPWR to wake up the waiting thread
          Real::pthread_kill(thread->self, SIGPWR);
        }

        //wait other threads to finish wakeup
        // while(thread->action != E_THREAD_ACTION_NONE){
        //  wait_thread_epochbegin(thread);  
        // }

        // Otherwise, we will let the application logic to wake it up, but with the necessary action.
        unlock_thread(thread);
      }
		}
	}
	
	PRINF("After waking up non-waiting threads at epoch begin\n");	
}

#ifdef DETECT_USAGE_AFTER_FREE
bool xthread::addQuarantineList(void* ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}
#endif

void xthread::checkRollbackCurrent() {
	// Check whether I should go to sleep or not.
  lock_thread(current);
  if(current->isNewlySpawned) {
    while(current->action != E_THREAD_ACTION_ROLLBACK) {
      // FIXME Hongyu 
      // if this thread is the coordinater thread, 
      // and wakeup other thread to rollback,
      // one other thread invokes another rollback.
      // This thread status is not set.  
    	wait_thread(current);
		}
	}
  unlock_thread(current);

	rollbackCurrent();
}

void xthread::rollbackCurrent() {
	// Setting the current status
  current->status = E_THREAD_RUNNING;
  current->action = E_THREAD_ACTION_NONE;

#ifdef DETECT_USAGE_AFTER_FREE
  current->qlist.restore();
#endif
  PRINF("xthread %d::rollback now\n", current->index);
  // Recover the context for current thread.
  restoreContext();
}

//This function is used by stop
void xthread::releaseHoldingLocks(pthread_mutex_t* mutex){
  // release or acquire locks won't happen during rollback, since customized lock is applied
  if(global_isRollback()){ return; }

  int lockNumber = current->lockcounter;
  if(mutex != NULL) lockNumber--;
  // if thread doesn't hold locks, just return
  // if holding lock number is greater than event number, bugs
  if(lockNumber<=0){
    return;
  }

  //traverse lockset in last epoch, some lock may exist in two or more epochs
  for(int i=0; i<LOCKSETSIZE; i++){
    syncEvent* event = &current->lockset[i];
    if(event->thread != NULL && event->thread == current){
      if(event->synccmd >= E_SYNC_MUTEX_LOCK && event->synccmd <=E_SYNC_COND_WAIT) {
        pthread_mutex_t* realMutex = (pthread_mutex_t*)getSyncEntry(event->syncvar);
        if(!isInvalidSyncVar(realMutex) && realMutex->__data.__owner == current->tid){
          if(realMutex != mutex){
            Real::pthread_mutex_unlock(realMutex); 
            PRINF("--- thread %d release lock %p in lockset %d", current->index, event->syncvar, i);
            // decrement counter
            lockNumber--;
          }
        }else{
          PRINF("--- thread %d remove lock in lockset %d", current->tid, i);
          event->thread = NULL;
        }
      } 
      /*
      else if(synccmd == E_SYNC_RWLOCK_RDLOCK || synccmd == E_SYNC_RWLOCK_WRLOCK) {
        pthread_rwlock_t* realRWLock = (pthread_rwlock_t*)getSyncEntry(event->syncvar);
        if(!isInvalidSyncVar(realRWLock) && realRWLock->__data.__owner == current->tid){
          if(realRWLock != mutex){
            Real::pthread_mutex_unlock(realMutex); 
            //PRINT("thread %d release lock %p in lockset", current->index, current->lockset[i]);
            // decrement counter
            lockNumber--;
          }
        }else{
          //PRINT("thread %d remove %p in lockset", current->tid, current->lockset[i]);
          event->thread = NULL;
        }
      }
      */
    } else {
      // just in case, set it to null
      event->thread = NULL;
    }
  }
  
  size_t eventNum = current->syncevents.getEntriesNumb(); 
  if(eventNum == 0){
    if(lockNumber == 0){ 
      return;
    } else{ 
      PRINT("thread %d lock counter is not 0, event number is 0", current->index);
    }
  }
    
  //traverse all events inversely so that we can find the latest lock
  long lastLockIns = eventNum-1;
  struct syncEvent* event = NULL;
  // lockNumber indicates found lock here
  while(lockNumber!=0) { 

    if(lastLockIns<0){
      PRINT("thread %d lock counter is wrong", current->index);
      while(1);
      abort();
    } 
    // move to previous event
    event = (struct syncEvent*)current->syncevents.getEntry(lastLockIns--);
    assert(event!=NULL);

    // find lock event
    if(event->synccmd==E_SYNC_MUTEX_LOCK 
        || (event->synccmd==E_SYNC_MUTEX_TRY_LOCK && event->ret==0)){
#if TESTSEGV
      // skip invalid lock
      if(event->syncvar == NULL) {
        lockNumber--;
        continue;
      }
#endif
      // get the real lock variable
      pthread_mutex_t* realMutex = (pthread_mutex_t*)getSyncEntry(event->syncvar);

#if TESTSEGV
      if(realMutex == NULL){
			  realMutex = (pthread_mutex_t*)_sync.retrieveRealSyncEntry(E_SYNCVAR_MUTEX, event->syncvar);
      }
#endif

      // release or acquire lock
      // if current thread is holding lock, then release
      if(realMutex->__data.__owner==current->tid && realMutex != mutex){

        Real::pthread_mutex_unlock(realMutex);
        PRINF("--- thread %d release lock %p, real mutex %p", current->index, event->syncvar, realMutex);
        saveHoldingLocks(event);
        // decrement counter
        lockNumber--;
      }
    }

    /* 
     * if it is RW lock, we need to track that
     *
     * */
  }
}

//This function is used by checking stop or saving context 
void xthread::resetHoldingLocks(){
  // re-acquire lock again
  for(int i=0; i<LOCKSETSIZE; i++){
    syncEvent* event = &current->lockset[i];
    if(event->thread != NULL){
      // if there is no lock, just clean up it
      if(current->lockcounter != 0){
        pthread_mutex_t* realMutex = (pthread_mutex_t*)getSyncEntry(event->syncvar);
        Real::pthread_mutex_lock(realMutex);
        PRINF("+++ current->tid %d lock counter %d reset locks %p, real lock %p", current->tid, current->lockcounter, event->syncvar, realMutex);
      }else{
        event->thread = NULL;
#ifdef SINGLELL
        event->next = NULL;
#endif
      }
    }
  }
  // set per-lock variable list, so every lock list can keep holding lock
  memcpy(current->backuplockset, current->lockset, sizeof(syncEvent) * LOCKSETSIZE);
  for(int i=0; i<LOCKSETSIZE; i++){
    syncEvent* event = &current->backuplockset[i];
    if(event->thread != NULL){
      SyncEventList* list = (SyncEventList *)event->eventlist;
#ifdef SINGLELL
      list->appendEvent(event);
#else
      listInsertTail(&event->list, &list->list);
#endif
    }
  }

}

void xthread::saveHoldingLocks(syncEvent* curevent){
  // if it is a cond wait, we need to check whether we have already saved or not.
  // Because the thread can only wait on one variable at one time
  if(curevent->synccmd == E_SYNC_COND_WAIT) {
    PRINF("*** current->tid %d save locks %p for E_SYNC_COND_WAIT", current->tid, curevent->syncvar);
    for(int i=0; i<LOCKSETSIZE; i++){
      syncEvent* event = &current->lockset[i];
      // it is already saved
      if(event->thread == curevent->thread
          && event->syncvar == curevent->syncvar
          && event->synccmd == curevent->synccmd
          && event->eventlist == curevent->eventlist) {
        return;
      }
    }
  }

  for(int i=0; i<LOCKSETSIZE; i++){
    syncEvent* event = &current->lockset[i];
    if(event->thread == NULL){
      memcpy(event, curevent, sizeof(struct syncEvent));
#ifdef SINGLELL
      event->next = NULL;
#endif
      break;
    }
  }
}
