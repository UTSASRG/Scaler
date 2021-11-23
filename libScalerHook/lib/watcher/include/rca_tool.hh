#if !defined(RCA_TOOL_H)
#define RCA_TOOL_H

#include "xdefines.hh"
#include "rca_disas.hh"
#include "software_breakpoint.h"

/***
 * Root Cause Analysis Tool
 * */

class RCATool {

  public:
    static RCATool& getInstance() {
      static char buf[sizeof(RCATool)];
      static RCATool* theOneTrueObject = new (buf) RCATool();
      return *theOneTrueObject;
    }

    void startAnalysis(faultyType type, void* ip, int level, sFrame* callstack, void* context);

    void resumeAnalysis();

    // CHECK: whether current stage is finished
    // CHECK: whether we need track more
    bool checkFinish(); 
    
    // PRINT out all root causes
    void printReport();

    void installAllBreakPoint();

    void disableWatchPoint();

    void disableBreakPoint();

    int installBreakPoint(void* address, pid_t pid);
 
    trackctl* getTrackCtl() { return &ctl; }

    void syncPTToCurPosition();
      
    bool filterContext(void* inst, void* real_inst, int signum, siginfo_t* siginfo, void* context);

    void getMemoryAddressByPrevInstruction(void* addr, void* context, MemAccess* mem) {
      RCADisas::getInstance().getMemoryAddressByPrevInstruction(addr, context, mem); 
    }

    // Handling the signal SIGUSR2
    static void sigusr2Handler(int signum, siginfo_t* siginfo, void* context);
    
    // @brief Install a handler for SIGUSR2 signals.
    void installSigUsr2Handler();
 
    void SaveCallStackAndValue(int thread, void* addr, void* memoryaddr, long value, int level, sFrame* callstack, void* current_ctx);

    void SaveCurrentThreadSync(int* sync_num, SyncEvent** sync_event);

    void AnalysisHappenBeforeRelationship(int current_tid, int access_counter, CallStackInfo** write_context);

    int CompareHappenBeforeBetweenCallStack(int sync_num1, SyncEvent** sync_event1, int sync_num2, SyncEvent** sync_event2);

    CallStackInfo* CompareHappenBeforeBetweenThreads(CallStackInfo* csinfo1, CallStackInfo* csinfo2);

    int CompareHappenBeforeBetweenEvent(SyncEvent* sync_event1, SyncEvent* sync_event2);

#ifdef HAVE_SOFTWARE_BREAKPOINT
    breakpoint_t* FindBreakpointByAddress(void* address);
    
    void SoftwareBreakpointHandler(breakpoint_t* bp, int signum, siginfo_t* siginfo, void* context);
#endif

  private:

    RCATool() {

      // init ctl information
      memset(&ctl, 0, sizeof(trackctl));
      ctl.cmd = TRACK_CMD_NONE;

      // used to comfirm multiple assignments
      installSigUsr2Handler();

      memset(thread_access_buffer_, 0, sizeof(thread_access_buffer_));
    }

    thread_t* getThreadByTid(pid_t tid);

    // check whether it is in code section
    bool isValid(void *ip) {
      mapping* m = selfmap::getInstance().getMappingByAddress(ip);
      return m!=NULL && m->valid();
    }
    
    long perf_event_open(struct perf_event_attr* hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
      return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    }

    // track information
    trackctl ctl;

#ifdef HAVE_SOFTWARE_BREAKPOINT
    SoftwareBreakpoint sw_breakpoint;
#endif

    AllCallStackInfo thread_access_buffer_[xdefines::MAX_ALIVE_THREADS];
};

#endif
