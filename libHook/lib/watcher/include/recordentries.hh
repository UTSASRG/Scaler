#if !defined(DOUBLETAKE_RECORDENTRIES_H)
#define DOUBLETAKE_RECORDENTRIES_H

/*
 * @file   recordentires.h
 * @brief  Managing record entry for each thread. Since each thread will have different entries,
 There is no need to use lock here at all.
 The basic idea of having pool is
 to reduce unnecessary memory allocation and deallocation operations, similar
 to slab manager of Linux system. However, it is different here.
 There is no memory deallocation for each pool.
 In the same epoch, we keep allocating from
 this pool and udpate correponding counter, updating to next one.
 When epoch ends, we reset the counter to 0 so that we can reuse all
 memory and there is no need to release the memory of recording entries.

 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>   /*   For SYS_xxx definitions */

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"
#include "real.hh"

void invokeCommit();
void stopCurrentThread();
//Each thread will have a class liked this. Thus, we can avoid 
//memory allocation when we are trying to record a synchronization event.
//The total number of entries for each thread is xdefines::MAX_SYNCEVENT_ENTRIES.
//Thus, there is no need to reclaim it. 
//When an epoch is finished, we will call   
template <class Entry> class RecordEntries {
public:
  RecordEntries() {}

  void initialize(int entries) {
    void* ptr;

    // add one backup entry
    _size = alignup((entries+1) * sizeof(Entry), xdefines::PageSize);
    ptr = MM::mmapAllocatePrivate(_size);
    if(ptr == NULL) {
      PRWRN("%d fail to allocate sync event pool entries : %s\n", getpid(), strerror(errno));
      ::abort();
    }

    //  PRINF("recordentries.h::initialize at _cur at %p. memory from %p to 0x%lx\n", &_cur, ptr,
    // (((unsigned long)ptr) + _size));
    // start to initialize it.
    _start = (Entry*)ptr;
    _cur = 0;
    _total = entries;
    _iter = 0;
    return;
  }

  Entry* alloc() {
    Entry* entry = NULL;
	//	PRINF("allocEntry, _cur %ld\n", _cur);
    if(_cur < _total) {
      entry = (Entry*)&_start[_cur];
      _cur++;
    } else {
      // There are no enough entries now; re-allocate new entries now.
      PRWRN("Not enough entries, now _cur %lu, _total %lu at %p!!!\n", _cur, _total, &_cur);
      invokeCommit();
      entry = (Entry*)&_start[_cur];
    }
    return entry;
  }

#ifdef DUMP_LOG_TODISK
  void dumpToDisk(FILE *fp){
    /*
    char filename[MAX_FILENAME_LEN];
    fprintf(stderr, "%s_%ld_eventlist.log: cur %ld, total %ld\n", program_invocation_name, syscall(__NR_gettid), _cur, _total);
    snprintf(filename, MAX_FILENAME_LEN, "%s_%ld_eventlist.log", program_invocation_name, syscall(__NR_gettid));

    FILE *fp = fopen(filename, "ab+");
    */

    Real::fwrite(&_cur, 1, sizeof(size_t), fp);
    Real::fwrite((void*)_start, _cur, sizeof(Entry), fp);

    /*
    fclose(fp);
    */

    _cur = 0;
  }
#endif

  bool checkSlots(){
    bool ret = true;
    if((_total - _cur) < xdefines::BACKUP_SYSCALL_ENTRIES){
      ret = false;
    }
    return ret;
  }

  void cleanup() {
    _iter = 0;
    _cur = 0;
  }

  //FIXME keep lock synvevent 
  void prepareRollback() { _iter = 0; }

  inline Entry* getEntry(size_t index) { return &_start[index]; }

	void advanceEntry() {
    _iter++;
	}

  Entry* retrieveIterEntry() {
    Entry* entry = NULL;
    if(_iter < _cur) {
      entry = getEntry(_iter);
      _iter++;
    }
    return entry;
  }

  // No change on iteration entry. getEntry() is only invoked in re-execution phases.
  Entry* getEntry() {
    Entry* entry = NULL;
		PRINF("getEntry: _iter %ld, _cur %ld\n", _iter, _cur);
    if(_iter < _cur) {
      entry = getEntry(_iter);
    }
		else {
				// Now there is existing entry for the current thread
				// We will have to stop current thread
				stopCurrentThread();
		}
    return entry;
  }

  // Only called in the replay
  Entry* firstIterEntry() { 
    Entry* entry = (Entry*)&_start[_iter];
    if(_iter >= _cur) {
      memset(entry, 0, sizeof(Entry)); 
    }
    return entry;
  }

  size_t getEntriesNumb() { return _cur; }

private:
  Entry* _start;
  size_t _total;
  size_t _cur;
  size_t _size;
  size_t _iter;
};

#endif
