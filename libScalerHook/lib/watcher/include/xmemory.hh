#if !defined(DOUBLETAKE_XMEMORY_H)
#define DOUBLETAKE_XMEMORY_H

/*
 * @file   xmemory.h
 * @brief  Memory management for all.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <execinfo.h>

#include <new>

#include "globalinfo.hh"
#include "internalheap.hh"
#include "log.hh"
#include "memtrack.hh"
#include "objectheader.hh"
#include "real.hh"
#include "selfmap.hh"
#include "threadstruct.hh"
#include "watchpoint.hh"
#include "xdefines.hh"
#include "xglobals.hh"
#include "xheap.hh"
#include "xoneheap.hh"
#include "xpheap.hh"
#include "xthread.hh"
#include "rca_tool.hh"
#include "backtrack.hh"

#include "pt_manager.h"

// Include all of heaplayers
#include "heaplayers.h"

// Encapsulates all memory spaces (globals & heap).
class xmemory {
private:
  // Private on purpose. See getInstance(), below.
  xmemory() {}

public:
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static xmemory& getInstance() {
    static char buf[sizeof(xmemory)];
    static xmemory* theOneTrueObject = new (buf) xmemory();
    return *theOneTrueObject;
  }

  void initialize() {
    // Install a handler to intercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();

    // Call _pheap so that xheap.h can be initialized at first and then can work normally.
    _heapBegin =
        (intptr_t)_pheap.initialize((void*)xdefines::USER_HEAP_BASE, xdefines::USER_HEAP_SIZE);

    _heapEnd = _heapBegin + xdefines::USER_HEAP_SIZE;
    _globals.initialize();
  }

  void finalize() {
    _globals.finalize();
    _pheap.finalize();
  }

  // add new global region
  inline void AddNewGlobalRegion(unsigned long begin, unsigned long end) { 
    _globals.AddNewRegion(begin, end);
  }

  inline int getGlobalRegionsNumb() { return _globals.getRegions(); }

  inline void getGlobalRegion(int index, unsigned long* begin, unsigned long* end) {
    _globals.getGlobalRegion(index, begin, end);
  }

  /* Heap-related functions. */
  inline void* malloc(size_t sz) {
    void* ptr = NULL;
    //PRINT("malloc, current %p ptr %p sz %ld\n", current, ptr, sz);

#ifdef DISABLE_THREADS 
		ptr = realmalloc(sz);
#else
    if(current->internalheap == true) {
      ptr = InternalHeap::getInstance().malloc(sz);
    } else {
	    ptr = realmalloc(sz);
    }
#endif
    //PRINT("malloc done, current %p ptr %p sz %ld\n", current, ptr, sz);
    return ptr;
  }

  inline void* calloc(size_t nmemb, size_t sz) {
    void* ptr = malloc(nmemb * sz);
    return ptr;
  }

	inline void setSentinels(void * ptr, size_t blockSize, size_t sz) {
		// Set sentinels by given the starting address and object size
    size_t offset = blockSize - sz;

    // P points to the last valid byte
    unsigned char* p = (unsigned char*)((intptr_t)ptr + sz);

    // If everything is aligned, add the guardzone.
    size_t nonAlignedBytes = sz & xdefines::WORD_SIZE_MASK;
    if(nonAlignedBytes == 0) {
      sentinelmap::getInstance().setSentinelAt(p);
     // PRINT("realmalloc at line %d\n", __LINE__);
    } else {

      // For those less than one word access, maybe we do not care since memory block is
      // always returned by 8bytes aligned or 16bytes aligned.
      // However, some weird test cases has this overflow. So we should detect this now.
     	unsigned char * startp = (unsigned char *)((intptr_t)p - nonAlignedBytes);
#if 1   
      size_t setBytes = xdefines::WORD_SIZE - nonAlignedBytes;
     // Check how much magic bytes we should put there.
     // The first byte of this is to specify how many bytes there.
     // For example, at 64bit, when nonAlignedBytes is 5,
     // Totally, we should put 3 bytes there.
     // We are using the first byte to mark the size of magic bytes.
     // It will end up with (02eeee).
			if(setBytes >= 2) {
        p[0] = setBytes - 1;
        for(size_t i = 1; i < setBytes; i++) {
          p[i] = xdefines::MAGIC_BYTE_NOT_ALIGNED;
        }
      } else if(setBytes == 1) {
        // PRINF("******setBytes %d\n", setBytes);
        p[0] = xdefines::MAGIC_BYTE_NOT_ALIGNED;
      }
    
		//	PRINT("setSentinels ptr %p sz %ld: nonAlignedBytes %ld startp %p p %p value %lx\n", ptr, sz, nonAlignedBytes, startp, p, *((unsigned long *)startp));
      sentinelmap::getInstance().markSentinelAt(startp);
#endif
      // We actually setup a next word to capture the next word
      if(offset > xdefines::WORD_SIZE) {
        void* nextp = (void*)((intptr_t)startp + xdefines::WORD_SIZE);
        sentinelmap::getInstance().setSentinelAt(nextp);
      }
    }
	}

  inline void* realloc(void* ptr, size_t sz) {
		if (ptr == NULL) {
    	ptr = malloc(sz);
    	return ptr;
  	}

  	if (sz == 0) {
    	free(ptr);
    	// 0 size = free. We return a small object.  This behavior is
    	// apparently required under Mac OS X and optional under POSIX.
    	return malloc(1);
		}

		// Now let's check the object size of this object
		objectHeader* o = getObject(ptr);

    // Get the block size
		size_t objSize = o->getObjectSize();

#ifdef DETECT_OVERFLOW
		size_t blockSize = o->getSize();
		if(blockSize >= sz) {
			if(!global_isRollback()) {
				// Check the object overflow.
      	if(checkOverflowAndCleanSentinels(ptr)) {
	#ifndef EVALUATING_PERF
      		PRWRN("iReplayer: Caught non-aligned buffer overflow error. ptr %p\n", ptr);
        	xthread::invokeCommit();
	#endif
				}
      }

			// Change the size of object to the new address
			o->setObjectSize(sz);

			setSentinels(ptr, blockSize, sz);
			return ptr;
		}
#endif
		
  	void * buf = malloc(sz);

  	if (buf != NULL) {
    	// Copy the contents of the original object
    	// up to the size of the new block.
    	size_t minSize = (objSize < sz) ? objSize : sz;
    	memcpy (buf, ptr, minSize);
  	}

  	// Free the old block.
  	free(ptr);

  	return buf;
  }

  // Actual allocations
  inline void* realmalloc(size_t sz) {

    unsigned char* ptr = NULL;
   	size_t mysize = sz;

    if(sz == 0) {
			sz = 1;
    }
		
		// Align the object size, which should be multiple of 16 bytes.
    if(sz < 16) {
      mysize = 16;
    }
		mysize = (mysize + 15) & ~15;

    ptr = (unsigned char*)_pheap.malloc(mysize);
#ifdef ENABLE_WATCHPOINT_DETECTOR
#ifdef DETECT_USAGE_AFTER_FREE
    if(global_isRollback()) {
      watchpoint::faultyObject *object = watchpoint::getInstance().findFaultyObjectByStartAddress(ptr);
      if (object != NULL && 
          object->objtype==OBJECT_TYPE_USEAFTERFREE && object->isMonitored==1) {
        PRINF("disable watchpoint at %p\n", ptr);
        watchpoint::getInstance().disableWatchpointByObject(object);
      }
    }
#endif
#endif

    //if(!ptr) return NULL;

    objectHeader* o = getObject(ptr);

    // Set actual size there.
    o->setObjectSize(sz);

#ifdef DETECT_OVERFLOW
    // Get the block size
    size_t size = o->getSize();

    assert(size >= sz);
    // Add another guard zone if block size is larger than actual size
    // in order to capture the 1 byte overflow.
//    PRINT("realmalloc at line %d size %ld sz %ld mysize %ld\n", __LINE__, size, sz, mysize);
    // Set actual size there.
    if(size > sz) {
			setSentinels(ptr, size, sz);
    }

    // Check the malloc if it is in rollback phase.
    if(global_isRollback()) {
      memtrack::getInstance().check(ptr, sz, MEM_TRACK_MALLOC);
    }
#endif

//		PRINT("realmalloc object %p\n", ptr);
    // We donot need to do anything if size is equal to sz
    //   PRINT("***********malloc object from %p to %lx. sz %lx\n", ptr, (unsigned long)ptr + sz,
    // sz);
    return ptr;
  }

  // We are trying to find the aligned address starting from ptr
  // to ptr+boundary.
  inline void* getAlignedAddress(void* ptr, size_t boundary) {
    return ((intptr_t)ptr % boundary == 0)
               ? ptr
               : ((void*)(((intptr_t)ptr + boundary) & (~(boundary - 1))));
  }

  inline void* memalign(size_t boundary, size_t sz) {
    // Actually, malloc is easy. Just have more memory at first.
    void* ptr = malloc(boundary + sz);

    // Since the returned ptr is not aligned, return next aligned address
    void* newptr = getAlignedAddress(ptr, boundary);

    // check offset between newptr and ptr
    size_t offset = (intptr_t)newptr - (intptr_t)ptr;
    if(offset == 0) {
      newptr = (void*)((intptr_t)newptr + boundary);
      offset = boundary;
    }

    // Check whether the offset is valid?
    assert(offset >= 2 * sizeof(size_t));
#if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKS)
    // Put a sentinel before the this memory block. We should do this
    sentinelmap::getInstance().setMemalignSentinelAt((void*)((intptr_t)newptr - sizeof(size_t)));
#endif

    // Put the offset before the sentinel too
    void** origptr = (void**)((intptr_t)newptr - 2 * sizeof(size_t));
    *origptr = ptr;

    PRINF("memalign: ptr %p newptr %p\n", ptr, newptr);
    return newptr;
  }

#ifdef DETECT_OVERFLOW
  bool checkOverflowAndCleanSentinels(void* ptr) {
    // Check overflows for this object
    objectHeader* o = getObject(ptr);

    // Get the block size
    size_t blockSize = o->getSize();

    // Set actual size there.
    size_t sz = o->getObjectSize();

    if(blockSize < sz) {
#ifndef EVALUATING_PERF
      PRWRN("Wrong object with blockSize %#lx sz %#lx\n", blockSize, sz);
      assert(blockSize >= sz);
#endif
    }

    return sentinelmap::getInstance().checkObjectOverflow(ptr, blockSize, sz, true);
  }
#endif

  // Get the actual pointer of memory block
  // Since we are putting a special magic number before this pointer
  // we can check the magic number.
  // If the magic number is corrupted, then we can find out later.
  void* getObjectPtrAtFree(void* ptr) {
    size_t* prevPtr = (size_t*)((intptr_t)ptr - sizeof(size_t));
    void* origptr = ptr;

    if(*prevPtr == xdefines::MEMALIGN_SENTINEL_WORD) {
      void** ppPtr = (void**)((intptr_t)ptr - 2 * sizeof(size_t));
#ifdef DETECT_OVERFLOW
      // Now we will cleanup the sentinel word.
      sentinelmap::getInstance().clearSentinelAt(prevPtr);
#endif
      origptr = *ppPtr;
    }
    return origptr;
  }

  bool inRange(intptr_t addr) { return (addr > _heapBegin && addr < _heapEnd) ? true : false; }

  // We should mark this whole objects with
  // some canary words.
  // Change the free operation to put into the tail of
  // list.
  void free(void* ptr) {
    void* origptr;

    if(!inRange((intptr_t)ptr)) {
      return;
    }

    // Check whether this is an memaligned object.
    origptr = getObjectPtrAtFree(ptr);
    objectHeader* o = getObject(origptr);

#ifndef EVALUATING_PERF
    // Check for double free
    if(o->isObjectFree()) {
      PRWRN("iReplayer: Caught double free or invalid free error. ptr %p\n", ptr);
      printCallsite();
    }
#endif

#ifdef DETECT_OVERFLOW
    // If this object has a overflow, we donot need to free this object
    if(!global_isRollback()) {
      if(checkOverflowAndCleanSentinels(origptr)) {
#ifndef EVALUATING_PERF
      	PRWRN("iReplayer: Caught buffer overflow error. ptr %p\n", origptr);
        xthread::invokeCommit();
#endif
        return;
      }
    }

    // Check the free if it is in rollback phase.
    if(global_isRollback()) {
      // PRERR("Check free on ptr %p size %d\n", ptr, o->getObjectSize());
      memtrack::getInstance().check(ptr, o->getObjectSize(), MEM_TRACK_FREE);
    }
#endif

    _pheap.free(origptr);

    // We remove the actual size of this object to set free on an object.
    o->setObjectFree();
   // PRINT("DoubleTake, line %d: free ptr %p\n", __LINE__, ptr);
    // Cleanup this object with sentinel except the first word.

#ifdef ENABLE_WATCHPOINT_DETECTOR
#ifdef DETECT_USAGE_AFTER_FREE
    if(global_isRollback()) {
      watchpoint::faultyObject *object = watchpoint::getInstance().findFaultyObjectByStartAddress(ptr);
      if (object != NULL && 
          object->objtype==OBJECT_TYPE_USEAFTERFREE && object->isMonitored==0) {
        PRINF("enable watchpoint at %p\n", ptr);
        watchpoint::getInstance().enableWatchpointByObject(object);
      }
    }
#endif
#endif

  }

  /// @return the allocated size of a dynamically-allocated object.
  inline size_t getSize(void* ptr) {
    // Just pass the pointer along to the heap.
    return _pheap.getSize(ptr);
  }

  // Commit something without checking for heap overflow.
  void atomicCommit(void* /* addr */, size_t /* size */) {
    //EDB FIXME why is this disabled?
#if 0
    if (inRange (addr)) {
      _pheap.commit (addr, size);
    }
#endif
  }

  /// Called when a thread need to rollback.
  inline void rollback() {

    // Release all private pages.
    _globals.recoverMemory();
    _pheap.recoverMemory();

    _pheap.recoverHeapMetadata();

#ifdef ENABLE_WATCHPOINT_DETECTOR
    // Now those watchpoints should be saved successfully,
    // We might have to install the watchpoints now.
    watchpoint::getInstance().installWatchpoints();
#endif
  }

  /// Rollback only without install watchpoints.
  inline void rollbackonly() {

    // Release all private pages.
    _globals.recoverMemory();
    _pheap.recoverMemory();

    // We should recover heap metadata in the end since
    // we will pass the position of heap inside recoverMemory.
    _pheap.recoverHeapMetadata();

#ifdef ENABLE_WATCHPOINT_DETECTOR
    // We do not need to install watch points if we only rollback.
    watchpoint::getInstance().installWatchpoints();
#endif
  }

  inline void printCallsite() {
    selfmap::getInstance().printCallStack();
   // PRINF("Program exited because of a double free or an invalid free.\n");
  //  exit(-1);
  }

  /// Transaction begins.
  inline void epochBegin() {
    _pheap.saveHeapMetadata();

    // Backup all existing data.
    _pheap.backup();
    _globals.backup();
  }

  inline void* getHeapEnd() { return _pheap.getHeapEnd(); }

  inline void* getHeapBegin() { return (void*)_heapBegin; }

  // This function is called before the system call is issued.
  inline bool checkOverflowBeforehand(void* start, size_t size) {
    bool hasProblem = false;

    // If it is not at heap, we do not care.
    if(_pheap.inRange(start)) {
      // Now we will check whether the final position should be in
      // the heap too.
      void* end = (void*)((intptr_t)start + size);
      if(_pheap.inRange(end)) {
        // check the possible overflow now.
        // We simply check whether this area has sentinels or not.
        // If there exists some sentinels there, it is a possible overflow.
        hasProblem = sentinelmap::getInstance().hasSentinels(start, size);
        PRINF("CANNOT write to an area with sentinels.\n");
      } else {
        hasProblem = true;
      }
    }

    return hasProblem;
  }

  // Check and commit in the end of transaction.
  inline bool checkHeapOverflow() {
    bool hasOverflow = false;

    // Whether it is a rollback phase
    if(global_isRollback()) {
      return false;
    }
		

#ifdef DETECT_OVERFLOW
    hasOverflow = _pheap.checkHeapOverflow();
#endif
//		PRINT("checkHeapOverflow: line %d hasOverflow %d\n", __LINE__, hasOverflow);
    // double elapse = stop(&startTime, NULL);
    if(hasOverflow == false) {
      // Check whether overflows and underflows have been detected
      // in the normal execution phase, like free()
      if(watchpoint::getInstance().hasToRollback()) {
        hasOverflow = true;
      }
    }
    return hasOverflow;
  }

  objectHeader* getObjectHeader(void* ptr) {
    objectHeader* o = (objectHeader*)ptr;
    return (o - 1);
  }

  // EDB: why is this here? Looks like a copy-paste bug (see above).
  static objectHeader* getObject(void* ptr) {
    objectHeader* o = (objectHeader*)ptr;
    return (o - 1);
  }

  void realfree(void* ptr);
  /// Rollback to previous
  static void crashHookForRollback();
  /* Signal-related functions for tracking page accesses. */

  /// @brief Signal handler to trap SEGVs.
  static void crashHandle(int signum, siginfo_t* siginfo, void* context) {

#ifdef GET_EPOCH_RUNTIME
    unsigned long end_time = getCurrentTime();
    PRINT("epoch time is %ld", end_time - global_rbSegvEpochStartTime());
    exit(-1);
#endif

    // if current thread is stopped,
    // we shold wait until the current epoch is finished.
    ucontext_t* segvcontext = (ucontext_t*)context;
    size_t* ip = (size_t*)segvcontext->uc_mcontext.gregs[REG_RIP]; 
    void* addr = siginfo->si_addr; // address of access

    // disable interception
    xthread::disableCheck();
    // disable pt first
    PTManager::getInstance().pt_DisableAll();

#ifdef TESTSEGV1 
    if(global_rbSegvNumber()==0){
      PRINT("thread %d Segmentation fault error\n", current->index);
      global_addrbSegvNumber();
      xrun::getInstance().rollbackDuringReexecution();
    }
    else{
      //bug is found in rollback
      g_bugReproduced = true;
      PRINT("%d times to reproduce segv fault\n", global_rbNumber());
      exit(0);
    }
#endif

    PRINT("%d: Signal %d received %d at inst %p, addr %p!\n", current->index, signum, siginfo->si_code, ip, addr);
#ifdef INTERACTIVE_DEBUG
    xrun::getInstance().stopAllThreads(); 
    if(!xthread::getInstance().checkSkipStatus()){
      xrun::getInstance().waitForUserCmd();
    }
#endif

   /* 
    selfmap::getInstance().printCallStack();
    PRINT("%d: Segmentation fault error %d at addr %p!\n", current->index, siginfo->si_code, addr);
    while(1) ;
    */

    // in case two threads have bugs
    xthread::enableCheck();
    // stop all
    xrun::getInstance().stopAllThreads(); 

    // disable interception
    xthread::disableCheck();
#ifdef GET_REPLAY_RUNTIME
    unsigned long current_time = getCurrentTime();
    PRINT("current time is %ld", current_time);
#endif
    if(global_rbSegvNumber()==0){
      PRINT("thread %d crash\n", current->index);
 
#ifdef GET_REPLAY_RUNTIME
      set_global_rbSegvStartTime();
#endif

      if(!xthread::getInstance().checkSkipStatus()){

        mapping* m = selfmap::getInstance().getMappingByAddress(ip);
        RCA_PRINF("ip is %p, start %lx (%s)\n", ip, m->getBase(), m->getFile().c_str());

        sFrame callstack[16];
        int fnum = my_backtrace(callstack, 15);

        /*
        char buf[256];
        for(int i=0; i<fnum; i++) {
          selfmap::getInstance().printCallStackByAddress(buf, i, (void*)callstack[i].regs[REG_RIP]);
        }
        */

#if 1
        if(signum == SIGSEGV) {
          RCATool::getInstance().startAnalysis(FAULTY_TYPE_SEGV, ip, fnum, callstack, context);
        } else if(signum == SIGABRT) {
          RCATool::getInstance().startAnalysis(FAULTY_TYPE_ABORT, ip, fnum, callstack, context);
        } else if(signum == SIGFPE) {
          RCATool::getInstance().startAnalysis(FAULTY_TYPE_FPE, ip, fnum, callstack, context);
        }
#endif
      }
    } else {
      //bug is found in rollback
      g_bugReproduced = true;
      if(!xthread::getInstance().checkSkipStatus()){
        RCATool::getInstance().resumeAnalysis();
        //while(1);
        //exit(-1);
      }
    }
    global_addrbSegvNumber();
    xthread::enableCheck();

    // Set the context to handleSegFault
    jumpToFunction((ucontext_t*)context, (unsigned long)xmemory::getInstance().crashHookForRollback);
    //Real::exit(-1);
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler() {
    static stack_t _sigstk;

    // Set up an alternate signal stack.
    _sigstk.ss_sp = MM::mmapAllocatePrivate(SIGSTKSZ);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    Real::sigaltstack(&_sigstk, NULL);

    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset(&siga.sa_mask);

    // Set the following signals to a set
    sigaddset(&siga.sa_mask, SIGSEGV);
    sigaddset(&siga.sa_mask, SIGABRT);
    sigaddset(&siga.sa_mask, SIGFPE);

    Real::sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
    //siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;
    siga.sa_flags = SA_SIGINFO | SA_RESTART;

    siga.sa_sigaction = xmemory::crashHandle;
    if(Real::sigaction(SIGSEGV, &siga, NULL) == -1) {
      printf("sfug.\n");
      exit(-1);
    }
    // assert() and abort() raise SIGABRT signal
    if(Real::sigaction(SIGABRT, &siga, NULL) == -1) {
      printf("sigaction SIGABRT fail.\n");
      exit(-1);
    }
    // divide by zero 
    if(Real::sigaction(SIGFPE, &siga, NULL) == -1) {
      printf("sigaction SIGFPE fail.\n");
      exit(-1);
    }

    Real::sigprocmask(SIG_UNBLOCK, &siga.sa_mask, NULL);
  }

#ifdef MEMORY_DUMP
  void saveMemToDisk(int flag){ _pheap.saveMemToDisk(flag); }
#endif

private:
  /// The globals region.
  xglobals _globals;

  intptr_t _heapBegin;
  intptr_t _heapEnd;

  /// The protected heap used to satisfy small objects requirement. Less than 256 bytes now.
  static xpheap<xoneheap<xheap>> _pheap;
};

#endif
