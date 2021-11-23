/*
 * @file   xrun.cpp
 * @brief  The main engine for consistency management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xrun.hh"

#include <assert.h>
#include <pthread.h>
#include <ucontext.h>

#include "globalinfo.hh"
#include "internalsyncs.hh"
#include "leakcheck.hh"
#include "syscalls.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xmemory.hh"

  /// @brief Initialize the system.
  void xrun::initialize() {
		struct rlimit rl;

    //		PRINT("xrun: initialization at line %d\n", __LINE__);
    // Check the stack size.
    __max_stack_size = xdefines::STACK_SIZE ;
	
	//set file number to max
	if(Real::getrlimit(RLIMIT_NOFILE, &rl) != 0) {
		PRWRN("Get the file number failed.\n");
		Real::exit(-1);
	}
//	rl.rlim_cur=rl.rlim_max;
  rl.rlim_cur = xdefines::MAX_OPENED_FILES/4;
  rl.rlim_max = xdefines::MAX_OPENED_FILES;
	if(Real::setrlimit(RLIMIT_NOFILE, &rl)) {
		PRINF("change files number limit failed, error %s\n", strerror(errno));
	}
	
	PRINF("NUMBER files limit: cur %ld, max %ld\n", rl.rlim_cur, rl.rlim_max);

#if 0 
    rl.rlim_cur = 524288;
    rl.rlim_max = 1048576;
    if(Real::setrlimit(RLIMIT_NOFILE, &rl)) {
      PRINF("change limit failed, error %s\n", strerror(errno));
    }
    PRINF("NUMBER files limit %d\n", rl.rlim_cur);

    while(1);
#endif

		// Initialize the locks and condvar used in epoch switches
    global_initialize();

    installSignalHandler();

    // Initialize the internal heap at first.
    InternalHeap::getInstance().initialize();

    xthread::getInstance().initialize();

    // Initialize the memory (install the memory handler)
    xmemory::getInstance().initialize();

    syscallsInitialize();
  }

  void xrun::reInitThreadInfo() {
    xthread::getInstance().reInitialize();
  }

  void xrun::finalize() {
#ifdef GET_REPLAY_RUNTIME
    unsigned long current_time = getCurrentTime();
	  fprintf(stderr, "replay times is %d\n", g_rbInSegvNumber);
	  fprintf(stderr, "runtime is %ld, starttime is %ld\n", current_time - g_rbInSegvStartTime, g_rbInSegvStartTime);
	  fprintf(stderr, "instructions %ld\n", trace_inst_count);
#endif
    PRINT("%d: finalize now !!!!!\n", getpid());
    // If we are not in rollback phase, then we should check buffer overflow.
    if(!global_isRollback() && !global_isEpochEnd()) {
#ifdef DETECT_USAGE_AFTER_FREE
      finalUAFCheck();
#endif

      epochEnd(true);
    }
	  else {
    	// Now we have to cleanup all semaphores.
    	//xthread::getInstance().finalize();
#ifdef MEMORY_DUMP
			xthread::disableCheck();
    	xmemory::getInstance().saveMemToDisk(1);//Hongyu save memory
    	xthread::enableCheck();
#endif
		//while(1) { ; }
		}
#ifdef GET_CHARECTERISTICS
	  fprintf(stderr, "epochs %ld\n", count_epochs);
	  fprintf(stderr, "actual epochs %ld\n", count_actual_epochs);
	  fprintf(stderr, "syscalls %ld\n", count_syscalls);
	  fprintf(stderr, "syncevent %ld\n", count_syncevent);
#endif

  }

void xrun::atomicCommit(void* addr, size_t size) { xmemory::getInstance().atomicCommit(addr, size); }
void xrun::syscallsInitialize() { syscalls::getInstance().initialize(); }

void xrun::rollback() {

  // During re-execution, it is possible that multiple threads trigger rollback at the same time.
  if(checkSkipStatus()){
	  xthread::getInstance().checkRollbackCurrent();
	  return;
  }
  
	PRINT("ROLLBACK now!\n");

  // Rollback all memory before rolling back the context.
#ifdef MEMORY_DUMP
  xthread::disableCheck();
  xmemory::getInstance().saveMemToDisk(0);//Hongyu	
  xthread::enableCheck();
#endif

	// Back all memory except the stack at first
  xmemory::getInstance().rollback();

  PRINF("\n\nAFTER MEMORY ROLLBACK!!!\n\n\n");

	// Set the phase to re-execution phase now.
	global_setRollback();
	 
  syscalls::getInstance().prepareRollback();
  // We must prepare the rollback, for example, if multiple
  // threads is existing, we must initiate the semaphores for threads
  // Also, we should setup those synchronization event list
  xthread::getInstance().prepareRollback();
   PRINF("xthread::getInstance() rollback and actual rollback\n");

  // Now we are going to rollback
  PRINF("\n\nSTARTING ROLLBACK!!!\n\n\n");

	// Now time to rollback myself.
  xthread::getInstance().checkRollbackCurrent();
 
	assert(0);
}

/// @brief Start a new epoch.
void xrun::epochBegin() {

  if(checkSkipStatus()){
    // If I am not the coordinator thread, we should wait the notification of the coordinator
	  lock_thread(current);

    current->status = E_THREAD_STOPPED;
    
    // We know that we will only been waken up once.
    // Thus, no need to check the status after wakenup
    wait_thread(current); 
    	
    // Since we met an irrevocable system call, we are not waiting anywhere 
    current->status = E_THREAD_RUNNING;
    current->action = E_THREAD_ACTION_NONE;
	  unlock_thread(current);
    
    // Currently, we have already performed irrevocable system call, then it is time to checkpoint.
	  xthread::saveContext();
	  return; 
  }

  threadmap::aliveThreadIterator i;
 
  PRINF("xrun epochBegin, joinning every thread.\n");
  
  unsigned long waitingThread = 0;  
	// Handle every thread correspondingly
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    lock_thread(thread);

    if(thread != current && thread->hasJoined) {
      PRINF("xrun, joining thread %d\n", thread->index);
      thread->action = E_THREAD_ACTION_EXIT;
      Real::pthread_cond_signal(&thread->cond);
      unlock_thread(thread);	

			// wait until the thread actually exit
      // Based on the manual, threads can join each other. But 
      // multithreads can not join the same thread simultaneously
      if(thread->isDetached != true) {
	      xthread::disableCheck();
	      Real::pthread_join(thread->self, NULL);
	      xthread::enableCheck();
      }
			else {
				// Add this thread to the removable list
				threadinfo::getInstance().deferSync(thread, E_SYNCVAR_THREAD);	
			}
    }
    else {
			// If the thread pointed by thread would like to enter the EpochEnd as the coordinator thread (current)
      if(thread->action == E_THREAD_ACTION_EPOCH_SKIP) {
        // This is very rare case. Thus, the performance is not the most important factor here. 
        thread->status = E_THREAD_RUNNING;

        // Notify the thread meeting with irrevocable system calls to perform system call and stop stably
        Real::pthread_cond_signal(&thread->cond);
        unlock_thread(thread);

        while(thread->status != E_THREAD_STOPPED) {
          // Avoid using the conditional variables since it creates
          // the complexity to know who is the coordinator 
          usleep(10);
        }
    
        lock_thread(thread);
      }

      // Since now we are in a new epoch mark all existing threads as old threads
    	thread->isNewlySpawned = false;

		  unlock_thread(thread);
      waitingThread++;
	  }
    	
    // cleanup the threads's different types of syscalls, qlist, pendingSyncevents, syncevents
    xthread::epochBegin(thread);
  }

  PRINF("xrun epochBegin, joinning every thread done.\n");
  // Cleanup some global lists.
	xsync::getInstance().epochBegin();

	// Now we will reset the positions of every opened files.	
  syscalls::getInstance().updateRecordableSyscalls();
  xthread::getInstance().cleanupSpawningList();

	// Run defered Syncs
  // Cleanup those threads inside the threadmap
  xthread::runDeferredSyncs();

  PRINF("xrun epochBegin, run deferred synchronizations done.\n");

  // Saving the context of the memory.
  xmemory::getInstance().epochBegin();

  // Now waken up all other threads then threads can do its cleanup.
  PRINF("getpid %d: xrun::epochBegin, wakeup others. \n", getpid());
  global_epochBegin();
  PRINF("getpid %d: xrun::epochBegin\n", getpid());

	// Now wakeup threads that are not waiting.
	xthread::wakeupThreadsAtEpochBegin(waitingThread);

#ifdef GET_REPLAY_RUNTIME
  set_global_rbSegvEpochStartTime();
#endif
  // Save the context of the current thread
	// We specifically save the context after doing all cleanups, since that won't 
	// be needed in re-execution 
  xthread::saveContext();

  PRINF("%d all threads start to a new epoch\n", current->index);

}

bool xrun::checkSkipBeforeStopAll() {
	return xthread::getInstance().checkSkipStatusAndActions();
}

bool xrun::checkSkipStatus(){
	return xthread::getInstance().checkSkipStatus();
}

void xrun::checkStopStatus() {
	xthread::getInstance().checkStopStatus();
}

void xrun::rollbackDuringReexecution(void) {
	PRINT("roll back during re-execution\n");
  // Now we should stop all threads at first.
  stopAllThreads();

	// Now we can rollback. Basically, it is very similar to epochEnd, 
	// but there is no need to check problems.
 	rollback(); 
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd(bool endOfProgram) {

#ifdef GET_CHARECTERISTICS
   __atomic_add_fetch(&count_epochs, 1, __ATOMIC_RELAXED);
#endif

  // To avoid endless rollback
  if(global_hasRollbacked()) {
#ifdef MEMORY_DUMP
	  sleep(3);
	  xthread::disableCheck();
	  xmemory::getInstance().saveMemToDisk(1);//Hongyu
	  xthread::enableCheck();
#endif
    PRINT("We have rollbacked before. Now let's stop!\n");	
    while(1) { ; }
  }
  else {
    // Wait for other threads to stop.
    stopAllThreads();

	  // check skip status
	  if(checkSkipStatus()) 
      return;

#ifdef GET_CHARECTERISTICS
   __atomic_add_fetch(&count_actual_epochs, 1, __ATOMIC_RELAXED);
#endif

#if defined(DETECT_OVERFLOW)
    bool hasOverflow = false;
    hasOverflow = xmemory::getInstance().checkHeapOverflow();
#endif

#if defined(DETECT_MEMORY_LEAKS)
    bool hasMemoryLeak = false;
    if(endOfProgram) {
      //  PRINF("DETECTING MEMORY LEAKAGE in the end of program!!!!\n");
      hasMemoryLeak =
        leakcheck::getInstance().doFastLeakCheck(xmemory::getInstance().getHeapBegin(), xmemory::getInstance().getHeapEnd());
    } else {
      // PRINF("DETECTING MEMORY LEAKAGE inside a program!!!!\n");
      hasMemoryLeak =
        leakcheck::getInstance().doSlowLeakCheck(xmemory::getInstance().getHeapBegin(), xmemory::getInstance().getHeapEnd());
    }
#endif

#ifndef EVALUATING_PERF
// First, attempt to commit.
#if defined(DETECT_OVERFLOW) && defined(DETECT_MEMORY_LEAKS)
    PRWRN("DoubleTake: in the end of an epoch, hasOverflow %d hasMemoryLeak %d\n", hasOverflow, hasMemoryLeak);
    if(hasOverflow || hasMemoryLeak) {
#ifdef INTERACTIVE_DEBUG
      waitForUserCmd();
#elif ENABLE_WATCHPOINT_DETECTOR
      rollback();
#endif
    } 
#elif defined(DETECT_OVERFLOW)
    PRINF("DoubleTake: in the end of an epoch, hasOverflow %d\n", hasOverflow);
    if(hasOverflow) {
#ifdef INTERACTIVE_DEBUG
      waitForUserCmd();
#elif ENABLE_WATCHPOINT_DETECTOR
      rollback();
#endif
    } 
#elif defined(DETECT_MEMORY_LEAKS)
    if(hasMemoryLeak) {
      // EDB FIX ME DISABLED
      // xmemory::getInstance().cleanupFreeList();
#ifdef INTERACTIVE_DEBUG
      waitForUserCmd();
#elif ENABLE_WATCHPOINT_DETECTOR
      rollback();
#endif
    }
#endif
#endif
  }
}

#ifdef INTERACTIVE_DEBUG
  void xrun::waitForUserCmd(){
    PRINT("Waiting for user commands (r: rollback, c: continue, e: exit):\n"); 
cmdcheck:
    int c = getchar();
    if(c == 'r'){
      rollback();
    }else if(c == 'c'){
      // do nothing
    }else if(c == 'e'){
      exit(0);
    }else{
      if(c != '\n'){
        PRINT("unknown command, try again!\n"); 
      }
      goto cmdcheck;
    }
  }
#endif

#ifdef DETECT_USAGE_AFTER_FREE
void xrun::finalUAFCheck() {
  threadmap::aliveThreadIterator i;
  // Check all threads' quarantine list
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    if(thread->qlist.finalUAFCheck()) {
      rollback();
    }
  }
}
#endif

void xrun::stopAllThreads() {
  // Stop all threads and set the phase to E_SYS_EPOCH_END. 
  setStopallAction();
		
  if(checkSkipBeforeStopAll()){
	  return;
  }

 // bool toprint = false;
  threadmap::aliveThreadIterator i;
  unsigned long totalRunningThreads; // Other threads that should be checked.
	bool safeInspection = true;

check_again:
  totalRunningThreads = 0; // Other threads that should be checked.

	// Traverse the thread map to check the status of every thread.
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

		// we only care about other threads
    if(thread != current) {
	  
			lock_thread(thread);
		  
			if(thread->status == E_THREAD_COND_TIMED_WAITING || thread->status == E_THREAD_EPOLL_WAITING){
			  thread->action = E_THREAD_ACTION_STOP;

        // Wake up the thread immediately
        // signal_waiting_thread(thread);
			
				unlock_thread(thread);
        
        while(thread->status != E_THREAD_STOPPED) {
          __asm__("pause"); 
        }
		  }
      else if(thread->status == E_THREAD_BLOCKING) {
        thread->action = E_THREAD_ACTION_STOP;
        Real::pthread_kill(thread->self, SIGRTMIN);
        unlock_thread(thread);
        while(thread->status != E_THREAD_STOPPED) {
          __asm__("pause"); 
        }
      }
			else if(thread->status == E_THREAD_WAITING_REEXECUTION){
			  xsync::signalThread(thread);
				
				unlock_thread(thread);

			  while((thread->status != E_THREAD_STOPPED)) { __asm__("pause"); }
		  }
			else{
				unlock_thread(thread);
			}
      
      if(thread->status == E_THREAD_RUNNING) {
        totalRunningThreads++;
      }

    }
  }
    
  if(totalRunningThreads != 0) {
//    PRINF("totalWaiters %lu, stableWaiters %lu, unstableWaiters %lu\n", totalWaiters, stableWaiters, unstableWaiters);
    // Now it is done.
		safeInspection = true;
    goto check_again;
  }else if(safeInspection){
		safeInspection = false;
    goto check_again;
	}

  // Set the phase to epoch end
  // Now all other threads should be stopped stably 
  global_setEpochEnd();
  PRINF("Thread %d Stop all threads succesfully!\n", current->index);
}

bool isNewThread() { return current->isNewlySpawned; }

void jumpToFunction(ucontext_t* cxt, unsigned long funcaddr) {
  PRINF("%p: inside signal handler %p.\n", (void*)pthread_self(),
      (void*)cxt->uc_mcontext.gregs[REG_RIP]);
  // selfmap::getInstance().printCallStack(NULL, NULL, true);
  cxt->uc_mcontext.gregs[REG_RIP] = funcaddr;
}

void invokeCommit() {
  xrun::getInstance().epochEnd(false);
  xrun::getInstance().epochBegin();
}

// This function is only called when there is no entry for system calls or sync events
// Thus, the current thread is either waiting forever or is about to rollback (when another thread finds a mismatch on the recorded event). 
void stopCurrentThread() {
 
  unsigned long totalRunningThreads = 0;
  
  global_spinlock();
  threadmap::aliveThreadIterator i;

  for(int double_check=0; double_check<2; double_check++) {
    totalRunningThreads = 0;
    for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
      thread_t* thread = i.getThread();

      if(thread != current) {
        lock_thread(thread);
        //if(thread->status != E_THREAD_STOPPED) {
        if(thread->status == E_THREAD_RUNNING) {
          totalRunningThreads++;
          //PRINT("thread %d, status %d", thread->index, thread->status);
        }
        unlock_thread(thread);
      }
    }
    usleep(100);
  }

  if(totalRunningThreads == 0){
    PRINT("All threads stoped in rollback\n");
    if(g_bugReproduced){
      PRINF("Bug has been reproduced, now exit\n");
      lock_thread(current);
      global_spinunlock();
      xthread::getInstance().stopCurrentThreadWithoutCheckpoint();
      unlock_thread(current);
    }else{
      global_spinunlock();
#ifdef TESTSEGV 
			xrun::getInstance().rollbackDuringReexecution();
#endif
    }

    exit(-1);
  }else{ 
    PRINF("stop current %d in rollback\n", current->index);
    lock_thread(current);
    global_spinunlock();
    xthread::getInstance().stopCurrentThreadWithoutCheckpoint();
    unlock_thread(current);
  }
}

void rollbackDuringReexecution(){
  xrun::getInstance().rollbackDuringReexecution();	
}
