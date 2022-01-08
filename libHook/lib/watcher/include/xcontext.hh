#if !defined(DOUBLETAKE_XCONTEXT_H)
#define DOUBLETAKE_XCONTEXT_H

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "log.hh"
#include "real.hh"
#include "xdefines.hh"

/**
 * @class xcontext
 * @brief User context to support the rollback mechanism.
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

class xcontext {
public:
  xcontext() {}

  void setupBackup(void* ptr, void* tmpptr) {
		//fprintf(stderr, "setupBackup ptr %p tempstack %p\n", ptr, tmpptr); 
		_backup = ptr; 
		_tmpStack=tmpptr; 
	}

  // void initialize(void * privateStart, void * privateTop, size_t totalPrivateSize, size_t
  // backupSize)
  void setupStackInfo(void* privateTop, size_t stackSize) {
    _privateTop = privateTop;
    _stackSize = stackSize;
  }

#if 0
  void rollback (bool stop) {
    /** There are two steps for this function.
     * First, we must recover the stack.
     * Second, we will setcontext to the saved context.
     * We should be careful about this function:
     *  a. If the saved stack is equal to or larger than current stack size, 
     *     then we can't simply overlap current stack from the saved stack.
     *     since it will screw current stack. 
     *  b. If the saved stack is smaller than current stacksize, then it can be
     *     safe to overlap current stack. 
     * In our implementation, we may utilize a temporary stack to recover the stack.
     */
    unsigned long ebp, esp;

    // The offset to the stack bottom.
    unsigned long espoffset, ebpoffset;

    unsigned long newebp, newesp;
    unsigned long stackStart;
    unsigned long offset;

    // Get current esp and ebp
#if defined(X86_32BIT)
    asm volatile(
      "movl %%ebp,%0\n" \
      "movl %%esp,%1\n" \
      :"=r"(ebp), "=r"(esp)
    );
#else
    asm volatile(
      "movq %%rbp,%0\n" \
      "movq %%rsp, %1\n" \
      :"=r"(ebp), "=r"(esp)
    );
#endif

    // Calculate the offset to stack bottom for ebp and esp register. 
    // Since we know that we are still using the original stack.
    espoffset = _stackTop - esp;
    ebpoffset = _stackTop - ebp;

    // Check whether we can utilize the temporary stack.
    if(espoffset > xdefines::TEMP_STACK_SIZE) {
      PRINF("Now esp %lx ebp %lx, stackbottom %lx\n", esp, ebp, _stackTop);
      PRINF("Now we can't use the reserved temporary stack, espoffset %lx temporary stack size %lx\n", espoffset, xdefines::TEMP_STACK_SIZE);
      // FIXME: we might use some malloced memory, but not now.
      abort();
    }

    // Calculate the new ebp and esp registers. 
    // We will set ebp to the bottom of temporary stack. 
    newebp = _tempStackBottom - ebpoffset;
    newesp = _tempStackBottom - espoffset;

    // Copy the existing stack to the temporary stack.
    // Otherwise, we can not locate those global variables???
    memcpy((void *)newesp, (void *)esp, espoffset);

    // Switch the stack manually. 
    // It is important to switch in this place (not using a function call), otherwise, the lowest
    // level of frame will be poped out and the stack will return back to the original one
    // Then techniquely we cann't switch successfully. 
    // What we want is that new frames are using the new stack, but we will recover
    // the stack in the same function later to void problems!!!
#if defined(X86_32BIT)
    asm volatile(
      // Set ebp and esp to new pointer
      "movl %0, %%ebp\n"
      "movl %1, %%esp\n"
      : : "r" (newebp), "r" (newesp)
    );
#else
    asm volatile(
      // Set ebp and esp to new pointer
      "movq %0,%%rbp\n"
      "movq %1,%%rsp\n"
      : : "r" (newebp), "r" (newesp)
    );
#endif 

    // The recovery of stack are safe no matter how large of the original stack
    memcpy(_pstackTopSaved, &_stack, _stackSize);
    
    if(stop) {
      while(1) ;
    }

    // After recovery of the stack, we can call setcontext to switch to original stack. 
    setcontext (&_context);
  }
#endif

  // Now we need to save the context
  inline void saveContext() {
    size_t size;
    // PRINF("SAVECONTEXT: Current %p _privateTop %p at %p _backup %p\n", getpid(), _privateTop,
    // &_privateTop, _backup);
    //    PRINF("saveContext nownow!!!!!!\n");
    // Save the stack at first.
    _privateStart = &size;
    size = size_t((intptr_t)_privateTop - (intptr_t)_privateStart);
    _backupSize = size;

//		PRINT("Saving stack to _backup %p from %p size %lx\n", _backup, _privateStart, size);

    if(size >= _stackSize) {
      PRWRN("Wrong. Current stack size (%lx = %p - %p) is larger than total size (%lx)\n", size,
            _privateTop, _privateStart, _stackSize);
      Real::exit(-1);
    }
    memcpy(_backup, _privateStart, size);
    // We are trying to save context at first
    getcontext(&_context);
  }

  // We already have context. How we can save this context.
  inline void saveSpecifiedContext(ucontext_t* context) {
    // Find out the esp pointer.
    size_t size;

    // Save the stack at first.
    _privateStart = (void*)getStackPointer(context);
    size = size_t((intptr_t)_privateTop - (intptr_t)_privateStart);
    _backupSize = size;

    if(size >= _stackSize) {
      PRWRN("Wrong. Current stack size (%lx = %p - %p) is larger than total size (%lx)\n", size,
            _privateTop, _privateStart, _stackSize);
      abort();
    }

    memcpy(_backup, _privateStart, size);

    // We are trying to save context at first
    memcpy(&_context, context, sizeof(ucontext_t));
  }

  /* Finish the following tasks here:
    a. Change current stack to the stack of newContext. We have to utilize
       a temporary stack to host current stack.
    b. Copy the stack from newContext to current stack.
    c. Switch back from the temporary stack to current stack.
    d. Copy the stack and context from newContext to oldContext.
    f. setcontext to the context of newContext.
   
  inline static void resetContexts(xcontext* oldContext, xcontext* newContext) {
    // We can only do this when these two contexts are for the same thread.
    assert(oldContext->getPrivateTop() == newContext->getPrivateTop());

    // We will backup the stack and context from newContext at first.
    oldContext->backupStackAndContext(newContext);

    restoreContext(oldContext, newContext);
  }
  

  // Copy the stack from newContext to oldContext.
  void backupStackAndContext(xcontext* context) {
    // We first backup the context.
    _privateStart = context->getPrivateStart();
    _backupSize = context->getBackupSize();

    memcpy(_backup, context->getBackupStart(), _backupSize);

    // Now we will backup the context.
    memcpy(&_context, context->getContext(), sizeof(ucontext_t));
  }
  */

  static void rollbackInsideSignalHandler(ucontext_t* context, xcontext* bakContext) {
    // We first rollback the stack.
		// Since the thread is inside the context of signal handler, we can simply 
		// recover the stack by copying, no need to worry about the correctness
    memcpy(bakContext->getPrivateStart(), bakContext->getBackupStart(),
           bakContext->getBackupSize());

    memcpy(context, bakContext->getContext(), sizeof(ucontext_t));
  }

  // Restore context from specified context.
  /* Finish the following tasks here:
    a. Change current stack to the stack of newContext. We have to utilize
       a temporary stack to host current stack.
    b. Copy the stack from current context to current stack.
    c. setcontext to the context of newContext.
   */
  inline static void restoreContext(xcontext* bakContext) {

    // Now we can mess with newContext.
    unsigned long ebp, esp;

    // The offset to the stack bottom.
    unsigned long espoffset, ebpoffset;
    unsigned long stackTop, newStackTop;
    unsigned long newebp, newesp;
// Get current esp and ebp
#if defined(X86_32BIT)
    asm volatile("movl %%ebp,%0\n"
                 "movl %%esp,%1\n"
                 : "=r"(ebp), "=r"(esp));
#else
    asm volatile("movq %%rbp,%0\n"
                 "movq %%rsp, %1\n"
                 : "=r"(ebp), "=r"(esp));
#endif

    // Calculate the offset to stack bottom for ebp and esp register.
    // Since we know that we are still using the original stack.
    stackTop = (unsigned long)bakContext->getPrivateTop();
    espoffset = stackTop - esp;
    ebpoffset = stackTop - ebp;

	//PRINT("stackTop %lx, espoffset %lx (esp %lx), ebpoffset %lx ebp %p\n", stackTop, espoffset, esp, ebpoffset, &ebp);
    REQUIRE(espoffset <= xdefines::TEMP_STACK_SIZE, "Temporary stack exhausted");

    // Calculate the new ebp and esp registers.
    // We will set ebp to the bottom of temporary stack.
    newStackTop = (intptr_t)bakContext->getTmpStack() + bakContext->getStackSize();
    newebp = newStackTop - ebpoffset;
    newesp = newStackTop - espoffset;

	//PRINT("tmpStack %lx, newstackTop %lx, newesp %lx, esp %lx\n",(intptr_t)bakContext->getTmpStack(), newStackTop, newesp, esp);
    // Copy the existing stack to the temporary stack.
    // Otherwise, we can not locate those global variables???
    memcpy((void*)newesp, (void*)esp, espoffset);
	//PRINT("memcopy newesp %lx, esp %lx, espoffset %lx\n", newesp, esp, espoffset);
	// Switch to the stack manually.
	// It is important to switch in this place (not using a function call), otherwise, the lowest
	// level of frame will be poped out and the stack will return back to the original one
	// Then techniquely we cann't switch successfully.
	// What we want is that new frames are using the new stack, but we will recover
	// the stack in the same function later to void problems!!!
#if defined(X86_32BIT)
    asm volatile(
        // Set ebp and esp to new pointer
        "movl %0, %%ebp\n"
        "movl %1, %%esp\n"
        :
        : "r"(newebp), "r"(newesp));
#else
    asm volatile(
        // Set ebp and esp to new pointer
        "movq %0,%%rbp\n"
        "movq %1,%%rsp\n"
        :
        : "r"(newebp), "r"(newesp));
#endif

//		PRINT("Recovery the stack: From %p, to %p\n size %lx\n", bakContext->getBackupStart(), bakContext->getPrivateStart(), bakContext->getBackupSize());

		//while(1) { ; } 
    // PRINF("________RESTORECONTEX___________at line %d\n", __LINE__);
    // Now we will recover the stack from the saved oldContext.
    memcpy(bakContext->getPrivateStart(), bakContext->getBackupStart(),
           bakContext->getBackupSize());

    //PRINF("Thread %p is calling actual setcontext", (void*)pthread_self());
    // After recovery of the stack, we can call setcontext to switch to original stack.
    setcontext(bakContext->getContext());
  }

  void* getStackTop() { return _privateTop; }

private:
  ucontext_t* getContext() { return &_context; }
  
  void* getTmpStack() { return _tmpStack; }
  
  void* getPrivateStart() { return _privateStart; }

  void* getPrivateTop() { return _privateTop; }

  size_t getBackupSize() { return _backupSize; }

  size_t getStackSize() { return _stackSize; }

  void* getBackupStart() { return _backup; }
  // How to get ESP/RSP from the specified context.
  unsigned long getStackPointer(ucontext_t* context) {
#ifndef X86_32BITS
    return context->uc_mcontext.gregs[REG_RSP];
#else
    return context->uc_mcontext.gregs[REG_ESP];
#endif
  }

  /// The saved registers, etc.
  ucontext_t _context;
  void* _backup; // Where to _backup those thread private information.
  void* _privateStart;
  void* _privateTop;
  void* _tmpStack;
  size_t _stackSize;
  size_t _backupSize;
};

#endif
