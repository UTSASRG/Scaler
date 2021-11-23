/*
 * @file   selfmap.h
 * @brief  Analyze the self mapping.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "selfmap.hh"

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.hh"
#include "xdefines.hh"
#include "xthread.hh"

#define MAX_BUF_SIZE 4096

// Normally, callstack only saves next instruction address.
// To get current callstack, we should substract 1 here.
// Then addr2line can figure out which instruction correctly
#define PREV_INSTRUCTION_OFFSET 1

// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
void selfmap::printCallStack() {
  void* array[256];
  int frames;

  // get void*'s for all entries on the stack
  xthread::disableCheck();
  frames = backtrace(array, 256);

	//char** syms = backtrace_symbols(array, frames);

  // Print out the source code information if it is a overflow site.
  selfmap::getInstance().printCallStack(frames, array);
  xthread::enableCheck();
}

void selfmap::printCallStackByAddress(char* buf, int index, void* addr) {
  //PRINT("\tThread %d: callstack frame %d: %p\t", current->index, i, addr);
  // Print out lines if the callsite is from the application or libireplayer64.so

  if(isApplication(addr)){ 
    PRINT("\tThread %d: callstack frame %d: %p (%s)\t", current->index, index, addr, _main_exe.c_str());
    sprintf(buf, "addr2line -a -i -e %s %p", _main_exe.c_str(), addr);
  } else {
    mapping* m = getMappingByAddress(addr);
    if(m && m->valid()){ 
      void* relative_address = (void*)((uintptr_t)addr - m->getBase()); 
      PRINT("\tThread %d: callstack frame %d: %p (%s)\t", current->index, index, relative_address, m->getFile().c_str());
      sprintf(buf, "addr2line -e %s %p", m->getFile().c_str(), relative_address); 
    }
  }

  system(buf);

  /* 
  if(isDoubleTakeLibrary((void *)addr)) {
    //continue;
    addr = (void *)((intptr_t)addr - (intptr_t)_doubletakeStart);
    PRINT("\tThread %d: callstack frame %d: %p (%s)\t", current->index, index, addr, _currentLibrary.c_str());
    sprintf(buf, "addr2line -e %s %p", _currentLibrary.c_str(), addr); 
  }
  else if(isPthreadLibrary((void *)addr)) {
    addr = (void *)((intptr_t)addr - (intptr_t)_libthreadStart);
    PRINT("\tThread %d: callstack frame %d: %p (%s)\t", current->index, index, addr, _threadLibrary.c_str());
    sprintf(buf, "addr2line -e %s %p", _threadLibrary.c_str(), addr);
  } 
  else {
    PRINT("\tThread %d: callstack frame %d: %p (%s)\t", current->index, index, addr, _main_exe.c_str());
    sprintf(buf, "addr2line -a -i -e %s %p", _main_exe.c_str(), addr);
  }
  */
}
  
// Calling system involves a lot of irrevocable system calls.
void selfmap::printCallStack(int frames, void** array) {
#if 0
	char** syms = backtrace_symbols(array, frames);

	for(int i=0; i<frames; i++) {
    fprintf(stderr, "  %d: %s\n", i, syms[i]);
  }
#endif

#if 1
	char buf[256];
  //  fprintf(stderr, "printCallStack(%d, %p)\n", depth, array);
  for(int i = 0; i < frames; i++) {
    void* addr = (void*)((unsigned long)array[i] - PREV_INSTRUCTION_OFFSET);
    printCallStackByAddress(buf, i, addr);
	}
#endif
#if 0
		// We print out the first one who do not belong to library itself
		//else if(index == 1 && !isDoubleTakeLibrary((void *)addr)) {
		else if(!isDoubleTakeLibrary((void *)addr)) {
    	index++;
      PRINT("\tcallstack frame %d: %p\n", index, addr);
		}
#endif
}
// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
int selfmap::getCallStack(void** array) {
  int size;

  PRINF("Try to get backtrace with array %p\n", array);
  // get void*'s for all entries on the stack
  xthread::disableCheck();
  size = backtrace(array, xdefines::CALLSITE_MAXIMUM_LENGTH);
  xthread::enableCheck();
  PRINF("After get backtrace with array %p\n", array);

  return size;
}
