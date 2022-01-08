#if !defined(SOFTWARE_BREAKPOINT_H)
#define SOFTWARE_BREAKPOINT_H
/*
 * =====================================================================================
 *
 *       Filename:  software_breakpoint.h
 *
 *    Description:  
 *
 *         Author: Hongyu Liu 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "xdefines.hh"
#include "threadstruct.hh"
#include "spinlock.hh"
#include "hashfuncs.hh"
#include "hashmap.hh"
#include "selfmap.hh"

#define BREAKPOINT_MAP_SIZE 4096
#define BREAKPOINT_MAX_SIZE 1024 

// the parameters are the same as signal handler
typedef void (*callback_sa_sigaction)(int, siginfo_t *, void *); 

typedef struct breakpoint_d {
  void* addr;
  // now it support one thread, 
  // we can change it to thread mask later
  pid_t thread_id; 
  char inst_data[15];
  int inst_length;

  // callback function
  callback_sa_sigaction func;

  mapping* text_mapping; 
} breakpoint_t;

class SoftwareBreakpoint {

  public:

    /*
    static SoftwareBreakpoint& getInstance() {
      static char swbuf[sizeof(SoftwareBreakpoint)];
      static SoftwareBreakpoint* theOneObject = new (swbuf) SoftwareBreakpoint();
      return *theOneObject;
    }
    */

    void SetBreakpoint(void* addr, pid_t thread_id, callback_sa_sigaction func);

    void SetAllBreakpoints(void** addr, int number, pid_t thread_id, callback_sa_sigaction func);

    void RemoveBreakpoint(void* addr);

    //void RemoveAllBreakpoint();

    void ModifyCodeSegment(char* dest, char* src, size_t size);

    void MakeCodeSegmentWritable(mapping* m);

    void MakeCodeSegmentReadOnly(mapping* m);

    breakpoint_t* FindBreakpointByAddress(void* address);

    SoftwareBreakpoint() {
      breakpoint_instruction[0] = 0xCC; // INT 3
      breakpoint_index = 0;
      pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
      active_breakpoint_.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, BREAKPOINT_MAP_SIZE);
      //InstallTrapHandler();
    }
  
  private:

    //static void SigTrapHandler(int signum, siginfo_t* siginfo, void* context);
    void SigTrapHandler(int signum, siginfo_t* siginfo, void* context);

    /*
    void InstallTrapHandler() {
      struct sigaction sig_trap;
      // block the signal, if it is already inside signal handler
      sigemptyset(&sig_trap.sa_mask);
      sigaddset(&sig_trap.sa_mask, SIGTRAP); 
      // set sa_sigaction handler
      sig_trap.sa_flags = SA_SIGINFO;
      sig_trap.sa_sigaction = SoftwareBreakpoint::SigTrapHandler;
      // register signal handler
      if(sigaction(SIGTRAP, &sig_trap, NULL) == -1) {
        fprintf(stderr, "setting breakpoint signal handler failed.\n");
        abort();
      }
    }
    */

    typedef HashMap<void*, breakpoint_t*, spinlock, InternalHeapAllocator> BpHashMap;
    BpHashMap active_breakpoint_;

    // the real breakpoint instruction
    char breakpoint_instruction[1];
    // an incremental number
    int breakpoint_index;
    // maximum number of breakpoint
    breakpoint_t bp_[BREAKPOINT_MAX_SIZE]; 

    pthread_spinlock_t lock_;
};

#endif
