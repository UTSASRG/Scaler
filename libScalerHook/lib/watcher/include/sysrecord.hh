#if !defined(DOUBLETAKE_RECORD_H)
#define DOUBLETAKE_RECORD_H

/*
 * @file   recordsyscalls.h
 * @brief  A simple list (First-In-First-Out) to record different types of information.
 *         This class only support two different kinds of syscall. One is PUSH, always
 *         insert an item into the back of list. Another is POP, always poping out the first
 *         item in the list.
 *         To avoid conflict, each thread should maintain all sorts of this information locally.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <dirent.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <errno.h>

#include "list.hh"
#include "log.hh"
#include "real.hh"
#include "recordentries.hh"
#include "xdefines.hh"

void stopCurrentThread();
void rollbackDuringReexecution();

class SysRecord {
  private:

    /**
     * In order to handle these record information uniformly, the first item is "list_t"
     */
    struct recordFile {
      list_t list;
      int fd;
      int ret;
    };

    struct recordMmap {
      void* addr;
    };

    struct recordMunmap {
      list_t list;
      void* addr;
      size_t length;
    };

    // sizeof(time_t) = 8
    struct recordTime {
      time_t time;
    };

    struct recordUmask {
      mode_t ret;
    };

    // sizeof(timeval) = 16, sizeof(timezone) = 8
    struct recordTimeofday {
      int ret;
      struct timeval tvalue;
      struct timezone tzone;
    };

    struct recordClockTime{
      clockid_t clk_id;
      struct timespec tp; 
      long ret;
    };

    // sizeof(clock_t) = 8, sizeof(tms) =32
    struct recordTimes {
      clock_t ret;
      struct tms buf;
    };

    struct recordSelect {
      int ret;
      void* data;
    };

    struct recordgetrlimit{
      int ret;
      int resource; 
      struct rlimit limit;
    };

    struct recordClone {
      int ret;
      pthread_t tid;
    };

    struct recordDir {
      list_t list;
      DIR* dir;
    };

    struct recordSock{
      list_t list;
      int fd;
      int ret;
      int size;
      int errcode;
      void* data;
    };

    struct recordSockInfo{
      list_t list;
      int fd; 
      int ret; 
      int errcode;
      void* data;
      struct sockaddr addr;
      socklen_t addrlen;
    };

    struct recordSockOpt{
      list_t list;
      int fd; 
      int ret; 
      char optval[16];
      socklen_t optlen;
    };

    struct recordSid {
      list_t list;
      int sid;
    };

    struct recordRet {
      list_t list;
      int ret;
      int errcode;
    };

    struct recordFdRet {
      list_t list;
      int fd;
      int ret;
    };

  public:

    void recordSid(eRecordSyscall sc, int sid) {
      struct recordSid* record = (struct recordSid*)allocRecordEntry(sc);
      record->sid = sid;
    }

    void getSid(eRecordSyscall sc, int* sid) {
      struct recordSid* record = (struct recordSid*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      *sid = record->sid;
    }

    // Record a file syscall according to given sc.
    void recordFileOps(eRecordSyscall sc, int fd) {
      recordFileOps(sc, fd, 0);
    }

    void recordFileOps(eRecordSyscall sc, int fd, int ret) {
      // using the assertion since only my code will call this.
      assert(sc <= E_SYS_FILE_DUP);

      struct recordFile* record = (struct recordFile*)allocRecordEntry(sc);
      record->fd = fd;
    }

    inline void deferFileClose(eRecordSyscall sc, int fd) {
      struct deferSyscall* entry = current->deferlist.alloc();
      entry->syscall = sc;
      entry->u.fd = fd;
    }

    // Get the fd with specific sc and remove corresponding item.
    void getFileOps(eRecordSyscall sc, int* fd) {
      assert(sc <= E_SYS_FILE_DUP);
      struct recordFile* record = (struct recordFile*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      *fd = record->fd;
      return;
    }

    // Record a file syscall according to given sc.
    void recordDirOps(eRecordSyscall sc, DIR* dir) {
      // using the assertion since only my code will call this.
      assert(sc == E_SYS_DIR_OPEN);

      struct recordDir* record = (struct recordDir*)allocRecordEntry(sc);
      record->dir = dir;
    }

    void deferDirClose(eRecordSyscall sc, DIR* dir) { 
      struct deferSyscall* entry = current->deferlist.alloc();
      entry->u.dirp = dir;
      entry->syscall = E_SYS_DIR_CLOSE; 
    }

    // Get the fd with specific sc and remove corresponding item.
    bool getDirOps(eRecordSyscall sc, DIR** dir) {
      bool isFound = false;
      assert(sc == E_SYS_DIR_OPEN || sc == E_SYS_DIR_CLOSE);
      struct recordDir* record = (struct recordDir*)retrieveRecordEntry(sc);
      if(record) {
        *dir = record->dir;
        isFound = true;
      }
      return isFound;
    }

    // record time results
    void recordTimeOps(time_t time) {
      struct recordTime* record = (struct recordTime*)allocRecordEntry(E_SYS_TIME);
      record->time = time;
    }

    // Get the first time results
    bool getTimeOps(time_t* time) {
      bool isFound = false;
      struct recordTime* record = (struct recordTime*)retrieveRecordEntry(E_SYS_TIME);

      if(record) {
        *time = record->time;
        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordgetrlimitOps(int ret, int resource, struct rlimit* limit) {
      struct recordgetrlimit* record = (struct recordgetrlimit*)allocRecordEntry(E_SYS_GETRLIMIT);
      record->ret = ret;
      record->resource = resource;
      if(limit){
        memcpy(&record->limit, limit, sizeof(struct rlimit));
      }
    }

    // Get the getrlimit results
    bool getrlimitOps(int* ret, int resource, struct rlimit* limit) {
      bool isFound = false;
      struct recordgetrlimit* record = (struct recordgetrlimit*)retrieveRecordEntry(E_SYS_GETRLIMIT);

      if(record && record->resource == resource) {
        *ret = record->ret;
        memcpy(limit, &record->limit, sizeof(struct rlimit));
        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordMmapOps(void* addr) {
      struct recordMmap* record = (struct recordMmap*)allocRecordEntry(E_SYS_MMAP);
      record->addr = addr;
    }

    // Get the first time results
    bool getMmapOps(void** addr) {
      struct recordMmap* record = (struct recordMmap*)retrieveRecordEntry(E_SYS_MMAP);
      bool isFound = false;

      if(record) {
        *addr = record->addr;
        isFound = true;
      }

      return isFound;
    }

    // record time results
    void deferMunmapOps(void* addr, size_t length) {
      // Adding this munmap to the list.
      struct deferSyscall* entry = current->deferlist.alloc();
      entry->u.munmap.addr = addr;
      entry->u.munmap.length = length; 
      entry->syscall = E_SYS_MUNMAP; 
    }

    // record time results
    void recordGettimeofdayOps(int ret, struct timeval* tv, struct timezone* tz) {
      struct recordTimeofday* record = (struct recordTimeofday*)allocRecordEntry(E_SYS_GETTIMEOFDAY);

      // Save those tv and tz
      if(tv) {
        memcpy(&record->tvalue, tv, sizeof(struct timeval));
      }
      if(tz) {
        memcpy(&record->tzone, tz, sizeof(struct timezone));
      }
      record->ret = ret;
    }

    // Get the first time results
    bool getGettimeofdayOps(int* ret, struct timeval* tv, struct timezone* tz) {
      struct recordTimeofday* record = (struct recordTimeofday*)retrieveRecordEntry(E_SYS_GETTIMEOFDAY);
      bool isFound = false;

      if(record) {
        // memcpy
        if(tv) {
          memcpy(tv, &record->tvalue, sizeof(struct timeval));
        }
        if(tz) {
          memcpy(tz, &record->tzone, sizeof(struct timezone));
        }
        *ret = record->ret;

        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordUTime(int ret) {
      struct recordTimes* record = (struct recordTimes*)allocRecordEntry(E_SYS_UTIME);
      record->ret = ret;
    }

    // Get the first time results
    bool getUTime(int* ret) {
      struct recordTimes* record = (struct recordTimes*)retrieveRecordEntry(E_SYS_UTIME);
      bool isFound = false;

      if(record) {
        // memcpy
        *ret = record->ret;
        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordTimesOps(clock_t ret, struct tms* buf) {
      struct recordTimes* record = (struct recordTimes*)allocRecordEntry(E_SYS_TIMES);

      // Save those tv and tz
      record->ret = ret;
      if(ret != -1) {
        memcpy(&record->buf, buf, sizeof(struct tms));
      }
    }

    // Get the first time results
    bool getTimesOps(clock_t* ret, struct tms* buf) {
      struct recordTimes* record = (struct recordTimes*)retrieveRecordEntry(E_SYS_TIMES);
      bool isFound = false;

      if(record) {
        // memcpy
        *ret = record->ret;
        if(buf) {
          memcpy(buf, &record->buf, sizeof(struct tms));
        }

        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordClockTimeOps(clockid_t clk_id, struct timespec *tp, long ret) {
      struct recordClockTime* record = (struct recordClockTime*)allocRecordEntry(E_SYS_CLOCKTIME);

      record->ret = ret;
      if(ret != -1) {
        record->clk_id = clk_id;
        memcpy(&record->tp, tp, sizeof(struct timespec));
      }
    }

    bool getClockTimeOps(clockid_t* clk_id, struct timespec *tp, long* ret) {
      struct recordClockTime* record = (struct recordClockTime*)retrieveRecordEntry(E_SYS_CLOCKTIME);
      bool isFound = false;

      if(record) {
        *ret = record->ret;
        if(*ret != -1) {
          *clk_id = record->clk_id;
          memcpy(tp, &record->tp, sizeof(struct timespec));
        }
        isFound = true;
      }

      return isFound;
    }

    // record time results
    void recordCloneOps(int ret, pthread_t tid) {
      struct recordClone* rc = (struct recordClone*)allocRecordEntry(E_SYS_CLONE);
      if(rc) {
        rc->ret = ret;
        rc->tid = tid;
      }
    }

    // Get the first time results
    bool getCloneOps(pthread_t* tid, int* ret) {
      bool isFound = false;
      struct recordClone* rc = (struct recordClone*)retrieveRecordEntry(E_SYS_CLONE);

      if(rc) {
        *tid = rc->tid;
        *ret = rc->ret;
        isFound = true;
      }

      return isFound;
    }

    void recordWritevLen(eRecordSyscall sc, int ret) {
      struct recordRet* record = (struct recordRet*)allocRecordEntry(sc);
      record->ret = ret;
    }

    int getWritevLen(eRecordSyscall sc) {
      struct recordRet* record = (struct recordRet*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      return record->ret;
    }

    void recordSetLock(eRecordSyscall sc, int ret, int errcode) {
      struct recordRet* record = (struct recordRet*)allocRecordEntry(sc);
      record->ret = ret;
      record->errcode = errcode;
    }

    int getSetLock(eRecordSyscall sc, int* errcode) {
      struct recordRet* record = (struct recordRet*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      *errcode = record->errcode;
      return record->ret;
    }

    // record time results
    void saveUmask(mode_t ret) {
      struct recordUmask* record = (struct recordUmask*)allocRecordEntry(E_SYS_UMASK);
      record->ret = ret;
    }

    // Get the first time results
    bool getUmask(mode_t* ret) {
      struct recordUmask* record = (struct recordUmask*)retrieveRecordEntry(E_SYS_UMASK);
      bool isFound = false;

      if(record) {
        *ret = record->ret;
        isFound = true;
      }

      return isFound;
    }

    //***********socket operations***********

    // record pipe  
    void recordPairOps(eRecordSyscall sc, int fd[2], int ret) {
      // we need save two pipe fd and one return value
      // since we do not change data structure, use size to store another fd
      recordSocketOps(sc, fd[0], NULL, fd[1], ret);
    }

    // Record a file syscall according to given sc.
    //record new
    void recordSocketOps(eRecordSyscall sc, int fd) {
      recordSocketOps(sc, fd, NULL, 0);
    }

    //record conn
    void recordSocketOps(eRecordSyscall sc, int fd, int ret) {
      recordSocketOps(sc, fd, NULL, 0, ret);
    }

    //record recv
    void recordSocketOps(eRecordSyscall sc, int fd, void* data, int size) {
      recordSocketOps(sc, fd, data, size, 0);
    }

    void recordSocketOps(eRecordSyscall sc, int fd, void* data, int size, int ret) {
      struct recordSock* record = (struct recordSock*)allocRecordEntry(sc);
      record->fd = fd;
      record->data = data;
      record->size = size;
      record->ret = ret;
      record->errcode = errno;
    }

    void recordSocketOps(eRecordSyscall sc, int fd, struct sockaddr* addr, socklen_t* addrlen) {
      struct recordSockInfo* record = (struct recordSockInfo*)allocRecordEntry(sc);
      record->fd = fd;
      record->errcode = errno;
      memcpy(&record->addr, addr, sizeof(struct sockaddr));
      memcpy(&record->addrlen, addrlen, sizeof(socklen_t));
    }

    void recordSocketOps(eRecordSyscall sc, int fd, struct sockaddr* addr, socklen_t* addrlen, int ret) {
      struct recordSockInfo* record = (struct recordSockInfo*)allocRecordEntry(sc);
      record->fd = fd;
      record->ret = ret;
      record->errcode = errno;
      memcpy(&record->addr, addr, sizeof(struct sockaddr));
      memcpy(&record->addrlen, addrlen, sizeof(socklen_t));
    }

    void recordSocketOps(eRecordSyscall sc, int fd, struct sockaddr* addr, socklen_t* addrlen, void* data, int ret) {
      struct recordSockInfo* record = (struct recordSockInfo*)allocRecordEntry(sc);
      record->fd = fd;
      record->ret = ret;
      record->data = data;
      record->errcode = errno;
      memcpy(&record->addr, addr, sizeof(struct sockaddr));
      memcpy(&record->addrlen, addrlen, sizeof(socklen_t));
    }

    void recordSockOpt(eRecordSyscall sc, int fd, int level, int optname, void* optval, socklen_t* optlen, int ret) {
      struct recordSockOpt* record = (struct recordSockOpt*)allocRecordEntry(sc);
      record->fd = fd;
      record->ret = ret;
      if(E_SYS_SOCKET_SETOPT != sc){
        record->optlen = *optlen;
        int len = *optlen;
        if(len > 16) len = 16;
        memcpy(record->optval, optval, len);
      }
    }

    void recordDescAndStatus(eRecordSyscall sc, int fd, int ret){
      struct recordFdRet* record = (struct recordFdRet*)allocRecordEntry(sc);
      record->fd = fd;
      record->ret = ret;
    }

    void getPairOps(eRecordSyscall sc, int fd[2], int* ret) {
      struct recordSock* record = (struct recordSock*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      fd[0] = record->fd;
      fd[1] = record->size;
      *ret = record->ret;
    }

    void getSocketOps(eRecordSyscall sc, int* fd) {
      struct recordSock* record = (struct recordSock*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      *fd = record->fd;
      errno = record->errcode;
    }

    void getSocketOps(eRecordSyscall sc, int* fd, int* ret) {
      struct recordSock* record = (struct recordSock*)retrieveRecordEntry(sc);
      assert(record != NULL); 
      *fd = record->fd;
      *ret = record->ret;
      errno = record->errcode;
    }

    void getSocketOps(eRecordSyscall sc, int fd, void **p, int* size) {
      struct recordSock* record = (struct recordSock*)retrieveRecordEntry(sc);
      assert(record != NULL);
      assert(record->fd==fd);
      *p = record->data;
      *size = record->size;
      errno = record->errcode;
    }

    void getSocketOps(eRecordSyscall sc, int* fd, struct sockaddr* addr, socklen_t* addrlen) {
      struct recordSockInfo* record = (struct recordSockInfo*)retrieveRecordEntry(sc);
      *fd = record->fd;
      errno = record->errcode;
      memcpy(addr, &record->addr, sizeof(struct sockaddr));
      memcpy(addrlen, &record->addrlen, sizeof(socklen_t));
    }

    void getSocketOps(eRecordSyscall sc, int fd, struct sockaddr* addr, socklen_t* addrlen, int* ret) {
      struct recordSockInfo* record = (struct recordSockInfo*)retrieveRecordEntry(sc);
      assert(fd==record->fd);
      *ret = record->ret;
      errno = record->errcode;
      memcpy(addr, &record->addr, sizeof(struct sockaddr));
      memcpy(addrlen, &record->addrlen, sizeof(socklen_t));
    }

    void getSocketOps(eRecordSyscall sc, int fd, void** data, struct sockaddr* addr, socklen_t* addrlen, int* ret) {
      struct recordSockInfo* record = (struct recordSockInfo*)retrieveRecordEntry(sc);
      assert(fd==record->fd);
      *ret = record->ret;
      *data = record->data;
      errno = record->errcode;
      //memcpy(addr, &record->addr, sizeof(struct sockaddr));
      memcpy(addrlen, &record->addrlen, sizeof(socklen_t));
    }

    int getSockOpt(eRecordSyscall sc, int fd, int level, int optname, void* optval, socklen_t* optlen) {  
      struct recordSockOpt* record = (struct recordSockOpt*)retrieveRecordEntry(sc);
      assert(fd==record->fd);
      if(E_SYS_SOCKET_SETOPT != sc){
        int len = record->optlen;
        if(len > 16) len = 16;
        memcpy(optval, record->optval, len);
        *optlen = record->optlen;
      }
      return record->ret;
    }

    int getDescAndStatus(eRecordSyscall sc, int fd){
      struct recordFdRet* record = (struct recordFdRet*)retrieveRecordEntry(sc);  
      assert(fd==record->fd);
      return record->ret;
    }

    // record select system call
    void recordSelect(int ret, void* data) {
      struct recordSelect* record = (struct recordSelect*)allocRecordEntry(E_SYS_SOCKET_SELECT);
      record->ret = ret;
      record->data = data;
    }

    // Get select results
    bool getSelect(int* ret, void** data) {
      struct recordSelect* record = (struct recordSelect*)retrieveRecordEntry(E_SYS_SOCKET_SELECT);
      bool isFound = false;

      if(record) {
        *ret = record->ret;
        *data = record->data;

        isFound = true;
      }

      return isFound;
    }

    //***************************************

    // Prepare the traverse for all list.
    static void prepareRollback(thread_t * thread) { thread->recordlist.prepareRollback(); }

  private:

    inline void* allocRecordEntry(eRecordSyscall sc) {
      struct recordSyscall* entry = current->recordlist.alloc();
      entry->syscall = sc;

      PRINF("SYSCALL: alloc entry %p for sc %d\n", entry, sc);
      return entry->data;
    }

    // For records, we only pop them from the beginning of a list
    void* retrieveRecordEntry(eRecordSyscall sc) {
      assert(sc >= E_SYS_FILE_OPEN && sc < E_SYS_MAX);

      struct recordSyscall* entry = current->recordlist.retrieveIterEntry();

      PRINF("SYSCALL: retrieveRecordEntry %p for sc %d\n", entry, sc);

      if(entry == NULL) {
        // Now it is the end of re-execution, making the current thread waiting on its conditional variable
        stopCurrentThread();
      } 
      else if(entry->syscall != sc) {
        PRINT("Unexpected condition happen, entry %p when required sc %d, rollback now\n", entry, sc);
        rollbackDuringReexecution();
      }

      return entry->data;
    }

};

#endif
