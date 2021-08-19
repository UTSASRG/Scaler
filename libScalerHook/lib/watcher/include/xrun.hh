#if !defined(DOUBLETAKE_XRUN_H)
#define DOUBLETAKE_XRUN_H

/*
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#include <execinfo.h>
#include <new>

#include "globalinfo.hh"
#include "internalheap.hh"
#include "log.hh"
#include "mm.hh"
#include "real.hh"
#include "watchpoint.hh"
#include "xdefines.hh"

#ifdef GET_CHARECTERISTICS
extern "C" {
	extern unsigned long count_epochs;
	extern unsigned long count_actual_epochs;
	extern unsigned long count_syscalls;
	extern unsigned long count_syncevent;
};
#endif
class xrun {

private:
  xrun()
     : _watchpoint(watchpoint::getInstance())
  {
    // PRINF("xrun constructor\n");
  }

public:

  static xrun& getInstance() {
    static char buf[sizeof(xrun)];
    static xrun* theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize();

  void reInitThreadInfo();
 
  void finalize();

#ifdef DETECT_USAGE_AFTER_FREE
  void finalUAFCheck();
#endif
  // Simply commit specified memory block
  void atomicCommit(void* addr, size_t size);

  /* Transaction-related functions. */
  /// Rollback to previous saved point
  void rollback();
  void checkStopStatus();
  bool checkSkipBeforeStopAll();
  bool checkSkipStatus();

  /// Rollback to previous
  void rollbackandstop();
  void rollbackDuringReexecution();

  void epochBegin();
  void epochEnd(bool endOfProgram);

  void stopAllThreads();

#ifdef INTERACTIVE_DEBUG
  void waitForUserCmd();
#endif
private:
  void syscallsInitialize();

  // SIGUSR1 signal handler
  static void sigusr1Handler(int signum, siginfo_t* siginfo, void* context) {
    // this is only used for handling this signal, 
    // otherwise, the default handler would be invoked and exit.
    /*
    PRINT("thread %d in sigusr1 handler", current->tid);
    void* array[256];
    int frames = backtrace(array, 256);
    int i;
    for(i = 0; i < frames; i++) {
      PRINT("%p", array[i]); 
    }
    */
  }

  // this is used for stopping thread waiting on blocking system call
  void installSignalHandler() {
    struct sigaction sigusr1;
    sigemptyset(&sigusr1.sa_mask);
    sigaddset(&sigusr1.sa_mask, SIGRTMIN);
    //sigusr1.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    // we do not use SA_RESTART, because we need to do checkpoint sometimes
    sigusr1.sa_flags = SA_SIGINFO;
    sigusr1.sa_sigaction = xrun::sigusr1Handler;
    if(Real::sigaction(SIGRTMIN, &sigusr1, NULL) == -1) {
      fprintf(stderr, "setting signal handler SIGUSR1 failed.\n");
      abort();
    }
  }

  // Notify the system call handler about rollback phase
  void startRollback();

private:
  /*  volatile bool _hasRolledBack; */
  watchpoint& _watchpoint;

  //  int   _rollbackStatus;
  /*  int _pid; // The first process's id. */
  /* int _main_id; */
};

#endif
