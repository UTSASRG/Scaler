#if !defined(DOUBLETAKE_THREADSTRUCT_H)
#define DOUBLETAKE_THREADSTRUCT_H

/*
 * @file   threadstruct.h
 * @brief  Definition of thread related structure.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <pthread.h>
#include <sys/types.h>

#include "list.hh"
#include "quarantine.hh"
#include "recordentries.hh"
#include "xcontext.hh"
#include "xdefines.hh"

typedef enum e_thrstatus {
  E_THREAD_STARTING = 0,
  E_THREAD_RUNNING,
  E_THREAD_JOINING, // The thread is trying to join other threads.
                    //    E_THREAD_EXITED, // The thread is exiting.
  E_THREAD_SIGNALED, // The thread has been signaled, waiting for the instruction
  E_THREAD_SIGWAITING,
  E_THREAD_EPOLL_WAITING, // Thre thread is waiting for a conditional variable
  //  E_THREAD_CONTINUE, // The thread should move forward.
  E_THREAD_COND_WAITING,// 6  // Thre thread is waiting for a conditional variable
  E_THREAD_COND_TIMED_WAITING, //Hongyu
  E_THREAD_BARRIER_WAITING,
  E_THREAD_WAITING_REEXECUTION, // Waiting on the semaphore in re-execution.
  E_THREAD_WAITFOR_JOINING, // The thread has finished and wait for the joining.
  E_THREAD_BLOCKING,

  // Thread are not exiting to guarantee the reproducibility
  // It marks its status E_THREAD_WAITFOR_REAPING, one thread
  // entering the committing phase should reap all wa
  E_THREAD_WAITFOR_REAPING, // status 11
  E_THREAD_EXITING,
  E_THREAD_STOPPED,
} thrStatus;

typedef enum threadAction {
  E_THREAD_ACTION_NONE = 0, // Default one, no need to do anything
  E_THREAD_ACTION_CHECKPOINT,
  E_THREAD_ACTION_ROLLBACK,
  E_THREAD_ACTION_STOP, // Only used in cond_timedwait, we require a stable waiting.
  E_THREAD_ACTION_EPOCH_SKIP, //FIXME TP
  E_THREAD_ACTION_EXIT, // The thread is notifed to exit.
} thrAction;

// System calls that will be recorded.
typedef enum e_recordSyscall {
  E_SYS_FILE_OPEN = 0,
  E_SYS_FILE_CLOSE,
  E_SYS_FILE_DUP,
  E_SYS_DIR_OPEN,
  E_SYS_DIR_CLOSE,
  E_SYS_MMAP,
  E_SYS_MUNMAP = 6, //6
  E_SYS_TIME,
  E_SYS_CLOCKTIME,
  E_SYS_GETTIMEOFDAY,
  E_SYS_GETRLIMIT,
  E_SYS_TIMES,
  E_SYS_UTIME, //12
  E_SYS_CLONE,
  E_SYS_CHMOD,
  E_SYS_FCHMOD,
  E_SYS_CHOWN,
  E_SYS_READ = 17,
  E_SYS_WRITE,
  E_SYS_FILE_FOPEN,
  E_SYS_FILE_FOPEN64,
  E_SYS_FILE_FCLOSE,
  E_SYS_LSEEK = 22,
  E_SYS_SIGPROCMASK,
  E_SYS_SIGACTION,
  E_SYS_PREAD,
  E_SYS_PWRITE,
  E_SYS_READV,
  E_SYS_WRITEV,
  E_SYS_FCNTL,
  E_SYS_FCNTL_SETLK,
  E_SYS_UMASK,
  E_SYS_RENAME,

  E_SYS_SOCKET_NEW, //33
  E_SYS_SOCKET_NAME,
  E_SYS_SOCKET_PEERNAME,
  E_SYS_SOCKET_CONN,
  E_SYS_SOCKET_RECV,
  E_SYS_SOCKET_RECVFROM,
  E_SYS_SOCKET_SEND,
  E_SYS_SOCKET_SENDMSG,
  E_SYS_SOCKET_EPOLL_CTL,
  E_SYS_SOCKET_BIND,
  E_SYS_SOCKET_LISTEN,
  E_SYS_SOCKET_SELECT, //44
  E_SYS_SOCKET_EVENT,
  E_SYS_SOCKET_PIPE,
  E_SYS_SOCKET_PAIR,
  E_SYS_SOCKET_GETOPT,
  E_SYS_SOCKET_SETOPT,

  E_SYS_DUP_SOCKET,
  E_SYS_DUP_FILE,

  E_SYS_SIGWAIT,
  
  E_SYS_SET_RLIMIT,
  E_SYS_SETSID,

  E_SYS_MAX
} eRecordSyscall;

// For the checking of the diverge, we only need 
// one simple entry;
struct simpleSyscall {
    unsigned long syscall;
}; 

struct recordSyscall {
    eRecordSyscall syscall;
    char data[64 - sizeof(eRecordSyscall)];
};

struct deferSyscall {
    eRecordSyscall syscall;
    union {
      int    fd;
      FILE * fp;
      DIR *  dirp;  
      struct {
        void * addr;
        size_t length;
      } munmap;
    } u;
}; 

typedef struct thread {
  bool available; // True: the thread index is free.
  bool internalheap;
  // Should we disable check or not?
  bool disablecheck;
  // bool      isSpawning; // Whether a new thread is spawning?
  bool isNewlySpawned; // whether this thread is spawned in this epoch?
  // Whether this thread has been joined or not.
  // If the thread has not been joined, then we can't reap this thread.
  // Otherwise, pthread_join may crash since the thread has exited/released.
  bool hasJoined;
  bool isSafe;   // whether a thread is safe to be interrupted
  // signal
  short sig;
  int index;
  pid_t tid;      // Current process id of this thread.
  pthread_t self; // Results of pthread_self

  // What is the status and action of a thread.
  thrStatus status;
  thrAction action;

  // If the thread is waiting on a user-provided conditional variable,  
  // we will record this conditional variable.
  pthread_cond_t * condwait;

  // mutex when a thread is trying to change its state.
  // In fact, this mutex is only protect joiner.
  // Only in the beginning of a thread (register),
  // we need to care about the joiner
  // protect critical part
  int lockcounter;
  int backup_lockcounter;
  syncEvent lockset[LOCKSETSIZE];
  syncEvent backuplockset[LOCKSETSIZE];

  pthread_mutex_t mutex;
  pthread_cond_t cond;
  
  // This is conditional variable used in epochBegin
  pthread_cond_t condepochbegin; 
  // Only for its joiner, which is introduced to avoid that the joiner and the current thread
  // may wait on the same conditional variable (in case of cond_timedwait).
  pthread_cond_t condjoiner;
  //TODO combine to reentry_spinlock
  pthread_spinlock_t spin; //Hongyu
  int spinlockcounter;
  int holdingSpinLockThread;

  // if a thread is detached, then the current thread don't need to wait its parent
  bool isDetached;

  // Local output buffer for each thread. In order to avoid malloc requests in the
  // replaying.
  char outputBuf[LOG_SIZE];

  // What is the parent of this thread
  struct thread* parent;

  struct thread* joiner;

  // System calls happens on this thread.
  // Cleanuplist is used to save those munmap, close, fclose, closedir lists.
  RecordEntries<struct recordSyscall> recordlist;
  RecordEntries<struct deferSyscall>  deferlist;
  RecordEntries<struct simpleSyscall> syslist;

  // Synchronization events happens on this thread.
  RecordEntries<struct syncEvent> syncevents;

#ifdef DETECT_USAGE_AFTER_FREE
  quarantine qlist;
#endif

  // struct syncEventList syncevents;
  //list_t pendingSyncevents;
  // struct syncEventList pendingSyncevents;

  // We used this to record the stack range
  void* stackBottom;
  void* stackTop;

  // Main thread have completely stack setting.
  bool mainThread;

  //semaphore sema;
  pthread_cond_t synccond;
  // We need to keep two context: one is old context, which is saved in the beginning of
  // transaction. another one is new context, which is normally saved in the signal handler.
  // For example, if one thread is going to commit, it is going to signal other threads to stop.
  // so we need another context to save context.
  xcontext bakContext;

#ifdef DUMP_LOG_TODISK
  // dump event list to disk
  FILE *fp;
#endif

  // for ENABLE_WATCHPOINT_DETECTOR
  int wpfd[4];

  // The following is the parameter about starting function.
  threadFunction* startRoutine;
  void* startArg;
  void* result;
} thread_t;

// The following structure will be added to alivelist
struct aliveThread {
  list_t list;
  thread_t* thread;
};

/*  
// A pending synchronization event needed to be handled by corresponding
// thread.
struct pendingSyncEvent {
  list_t list;
  struct syncEvent* event;
};
*/

// Each thread has corresponding status information in a global array.

// We will maintain an array about the status of each thread.
// Actually, there are two status that will be handled by us.
extern __thread thread_t* current;
extern bool initialized;

inline int getThreadIndex() {
#ifdef DISABLE_THREADS
  // For debugging purpose, we don't intercept all threads related functions
  return syscall(SYS_gettid)%xdefines::NUM_HEAPS;
#else
  if(initialized) {
    return current->index;
  } else {
    return 0;
  }
#endif
}
#endif
