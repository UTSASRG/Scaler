/*
 * =====================================================================================
 *
 *       Filename:  software_breakpoint.c
 *
 *    Description:  
 *
 *         Author: Hongyu Liu
 *   Organization:  
 *
 * =====================================================================================
 */

#include "software_breakpoint.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include "vm_emulator.h"

void SoftwareBreakpoint::SigTrapHandler(int signum, siginfo_t* siginfo, void* context) {
  ucontext_t* trapcontext = (ucontext_t*)context;
  size_t* inst = (size_t*)(trapcontext->uc_mcontext.gregs[REG_RIP] - 1); // address of breakpoint 
  void* nextinst = NULL;

  //breakpoint_t* bp = SoftwareBreakpoint::getInstance().FindBreakpointByAddress(inst);
  breakpoint_t* bp = FindBreakpointByAddress(inst);
  if(bp != NULL) {
    // this is a breakpoint

    pid_t current_thread = syscall(__NR_gettid);

    if(bp->thread_id == current_thread 
        && bp->func != NULL) {
      bp->func(signum, siginfo, context);
      fprintf(stderr, "I am current thread, inst is %p\n", inst);
    }

    // execute instruction on breakpoint
    nextinst = VmEmulator::getInstance().ExecuteInstruction(bp, context);

    fprintf(stderr, "Breakpoint, inst is %p, next inst is %p\n", inst, nextinst);
    // move to next instruction
    trapcontext->uc_mcontext.gregs[REG_RIP] = (intptr_t)nextinst;
  } else {
    // do something else  
  }
}

void SoftwareBreakpoint::SetAllBreakpoints(void** addr, int number, pid_t thread_id, callback_sa_sigaction func) {
  for(int i=0; i<number; i++) {
    SetBreakpoint(addr[i], thread_id, func);
  }
}

void SoftwareBreakpoint::SetBreakpoint(void* addr, pid_t thread_id, callback_sa_sigaction func) {

  breakpoint_t* bp;
  // !!! DOES NOT SUPPORT THE ADDRESS IN THE MIDDLE OF A INSTRUCTION !!!
  if(active_breakpoint_.find(addr, sizeof(bp), &bp)) {
    fprintf(stderr, "this address %p is already watched!\n", addr);
    return;
  }

  pthread_spin_lock(&lock_);
  do{
    int bp_idx = __atomic_add_fetch(&breakpoint_index, 1, __ATOMIC_RELAXED);
    bp_idx &= (BREAKPOINT_MAX_SIZE-1); 
    bp = &bp_[bp_idx];
  } while (bp->addr != NULL);

  bp->addr = addr;
  bp->thread_id = thread_id;
  bp->func = func;

  // set bp->inst_data, bp->inst_length
  bool is_instruction_valid = VmEmulator::getInstance().GetInstructionInformation(bp);
  fprintf(stderr, "set breakpoint (%p) at %p, thread is %d, is_instruction_valid %d, inst length is %d\n", 
      bp, bp->addr, bp->thread_id, is_instruction_valid, bp->inst_length);
  if(is_instruction_valid) {
    bp->text_mapping = selfmap::getInstance().getMappingByAddress(addr);

    bool is_exist = active_breakpoint_.insertIfAbsent(addr, sizeof(bp), bp);
    if(is_exist) {
      //TODO not support for multiple threads to set breakpoint on the same address 
      memset(bp, 0, sizeof(breakpoint_t));
    } else {
      // replace current instruction to INT 3
      MakeCodeSegmentWritable(bp->text_mapping);
      ModifyCodeSegment((char*)addr, breakpoint_instruction, 1);
      MakeCodeSegmentReadOnly(bp->text_mapping);
    }
  } else {
    memset(bp, 0, sizeof(breakpoint_t));
  }

  pthread_spin_unlock(&lock_);
}

void SoftwareBreakpoint::RemoveBreakpoint(void* addr) {
  breakpoint_t* bp;
  // !!! DOES NOT SUPPORT THE ADDRESS IN THE MIDDLE OF A INSTRUCTION !!!
  //if(!active_breakpoint_.erase(addr, sizeof(bp), &bp)) {
  if(!active_breakpoint_.EraseAndFetch(addr, sizeof(bp), &bp)) {
    fprintf(stderr, "this address is not watched!\n");
    return;
  }
 
  pthread_spin_lock(&lock_);

  // replace INT 3 to original instruction
  MakeCodeSegmentWritable(bp->text_mapping);
  ModifyCodeSegment((char*)addr, bp->inst_data, 1);
  MakeCodeSegmentReadOnly(bp->text_mapping);

  pthread_spin_unlock(&lock_);

  // clean up breakpoint data
  memset(bp, 0, sizeof(breakpoint_t));
}

void SoftwareBreakpoint::ModifyCodeSegment(char* dest, char* src, size_t size) {
  for(int i=0; i<size; i++) {
    *dest = *src;
  }
}

void SoftwareBreakpoint::MakeCodeSegmentWritable(mapping* m) {
  // PROT_READ | PROT_WRITE | PROT_EXEC 
  size_t len = m->getLimit() - m->getBase();
  mprotect((void*)m->getBase(), len, PROT_READ | PROT_WRITE | PROT_EXEC);
}

void SoftwareBreakpoint::MakeCodeSegmentReadOnly(mapping* m) {
  // PROT_READ | PROT_EXEC 
  size_t len = m->getLimit() - m->getBase();
  mprotect((void*)m->getBase(), len, PROT_READ | PROT_EXEC);
}

breakpoint_t* SoftwareBreakpoint::FindBreakpointByAddress(void* address) {
  breakpoint_t* bp;
  if(!active_breakpoint_.find(address, sizeof(bp), &bp)) {
    bp = NULL;
  }
  return bp;
}
