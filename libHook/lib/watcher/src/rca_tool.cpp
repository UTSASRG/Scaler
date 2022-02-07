
#include "rca_tool.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <syscall.h>
#include <ucontext.h>
#include <execinfo.h>

#include "xthread.hh"
#include "threadmap.hh"
#include "xmemory.hh"
#include "log.hh"
#include "pt_manager.h"
#include "rca_disas.hh"

#define PREV_INSTRUCTION_OFFSET 1

thread_t* RCATool::getThreadByTid(pid_t tid) {
  thread_t* ret = NULL;
  threadmap::aliveThreadIterator ti;
  for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
    thread_t* thread = ti.getThread();
    if(thread->tid == tid) {
      ret = thread;
      break;
    }
  }
  return ret;
}

void RCATool::startAnalysis(faultyType type, void* ip, int level, sFrame* callstack, void* context) {

  // skip callstack in our library
  int i = 0;
  while(callstack[i].regs[REG_RIP] != (intptr_t)ip) i++;
  memcpy(ctl.callstack, &callstack[i], (level-i) * sizeof(sFrame));
  memcpy(ctl.callstack[0].regs, ((ucontext_t *)context)->uc_mcontext.gregs, sizeof(gregset_t));
  //fprintf(stderr, "gregset_t is %zu, i=%d\n", sizeof(gregset_t), i);

  ctl.type = type;
  ctl.faultyip = ip;
  ctl.cslevel = level - i;
  ctl.current_level = 0;

  resumeAnalysis();
}

void RCATool::resumeAnalysis(){

  if(ctl.root_level != NULL) { 
    // close breakpoint
    Real::ioctl(ctl.root_level->breakpoint_fd, PERF_EVENT_IOC_DISABLE, 0);
    Real::close(ctl.root_level->breakpoint_fd);
  }

restart:
  switch(ctl.cmd) {
    case TRACK_CMD_NONE:
      {
        // save current position information in synchronization list
        SaveCurrentThreadSync(&(ctl.sync_num), ctl.sync_event);

        ctl.cmd = TRACK_CMD_TRACE;
        // start to collect pt trace
        RCA_PRINF("start to collect PT trace for all thread\n");
        PTManager::getInstance().pt_OpenAll();
        PTManager::getInstance().pt_EnableAll();

        break;
      }
    case TRACK_CMD_TRACE:
      {
        // disable tracking
        //PTManager::getInstance().pt_Disable();
        threadmap::aliveThreadIterator ti;
        for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
          thread_t* thread = ti.getThread();
          PTManager::getInstance().pt_ReadPTDataByThread(thread->index);
        }
        // stop pt trace
        PTManager::getInstance().pt_CloseAll();
        //PTManager::getInstance().pt_Print();

        //while(1);
        //exit(-1);
      }
    case TRACK_CMD_TRACE_ANALYSIS:
      {
        int thread_index = current->index;
        if(ctl.trace.cur != NULL) {
          thread_index = getThreadByTid(ctl.trace.cur->tid)->index;
        }
        // backword trace the faulty addresses
        RCADisas::getInstance().trackFaultyAddress(&ctl, PTManager::getInstance().getDecoderByThread(thread_index));

        // collect faulty instruction position 
        installAllBreakPoint();

        ctl.cmd = TRACK_CMD_CONTEXT;

        if(ctl.site.num_site != 0) {
          break;
        }
      }
    case TRACK_CMD_CONTEXT:
      {
        disableBreakPoint();

        RCADisas::getInstance().removeSpeculativeInst(&ctl);

        // compute the memory addresses
        RCADisas::getInstance().computeFaultyAddresses(&ctl);

        // install watchpoints
        int num_addr = ctl.site.num_site - 1;
        ctl.site.watched_site = ctl.site.num_site;
        int installed_wp = 0;
        for(int i=num_addr; i>=0; i--) {
          if(ctl.site.info[i]->faultyAddr != NULL) {
            if(installed_wp == xdefines::MAX_WATCHPOINTS) {
              break;
            }
            // align up the address, otherwise perf can not install
            void* addr = (void*)((intptr_t)ctl.site.info[i]->faultyAddr & ~0x3);
            PRINT("Suspicious memory address is %p, install watchpoint to track data flow!!!\n", addr);
            watchpoint::getInstance().addWatchpoint((void*)addr, *(unsigned int*)addr, OBJECT_TYPE_DATATRACE, (void*)addr, 0);
            installed_wp++;
          }
          ctl.site.watched_site--;
        }

        // clean up call stack information
        memset(thread_access_buffer_, 0, sizeof(thread_access_buffer_));

        if(ctl.site.watched_site == 0) {
          ctl.cmd = TRACK_CMD_DATAFLOW;
        } else {
          ctl.cmd = TRACK_CMD_DATAFLOW_CONT;
        }
        if(installed_wp != 0) {
          break;
        }
      }
    case TRACK_CMD_DATAFLOW_CONT:
      {
        disableWatchPoint();

        // install watchpoints
        int num_addr = ctl.site.watched_site - 1;
        int installed_wp = 0;
        for(int i=num_addr; i>=0; i--) {
          if(ctl.site.info[i]->faultyAddr != NULL) {
            if(installed_wp == xdefines::MAX_WATCHPOINTS) {
              break;
            }
            // align up the address, otherwise perf can not install
            void* addr = (void*)((intptr_t)ctl.site.info[i]->faultyAddr & ~0x3);
            PRINT("[CONTINUE] Suspicious memory address is %p, install watchpoint to track data flow!!!\n", addr);
            watchpoint::getInstance().addWatchpoint((void*)addr, *(unsigned int*)addr, OBJECT_TYPE_DATATRACE, (void*)addr, 0);
            installed_wp++;
          }
          ctl.site.watched_site--;
        }

        if(ctl.site.watched_site == 0) {
          ctl.cmd = TRACK_CMD_DATAFLOW;
        } else {
          ctl.cmd = TRACK_CMD_DATAFLOW_CONT;
        }
        if(installed_wp != 0) {
          break;
        }
      }
    case TRACK_CMD_DATAFLOW:
      {
        disableWatchPoint();

        printReport();

        if(checkFinish()) {
#ifdef GET_INST_COUNT 
          siteInfo* site = &ctl.trace.si[ctl.trace.num_site-1];
          int times = site->total_triggered_times;
          void* last_inst = site->instruction_address;
          //void* last_inst = (void*)0x41d34f; 
          PerThreadDecoder* decoder = PTManager::getInstance().getDecoderByThread(getThreadByTid(site->tid)->index);
          decoder->end_decode();
          decoder->start_decode();
          
          int meet_times = 0;
          size_t inst_count = 0;
          void* inst = decoder->pt_prev_instruction();
          while(inst) {
            if(inst == last_inst) {
              meet_times++;
              PRINT("times %d, meet %d times, inst %zu", times, meet_times, inst_count);
            }
            inst_count++;
            inst = decoder->pt_prev_instruction();
          } 
          PRINT("totla inst %zu", inst_count);
#endif
          exit(-1);
        } 
        
        // continue to analyze
        ctl.cmd = TRACK_CMD_TRACE_ANALYSIS;
        goto restart;

        break;
      }
    default:
      { }
  }

  if(ctl.root_level != NULL) { 
    ctl.root_level->breakpoint_fd = installBreakPoint(ctl.root_level->instruction_address, ctl.root_level->tid);
    RCA_PRINF("install breakpoint on thread %d at %p, fd %d, total_triggered_times %u\n", 
        ctl.root_level->tid, ctl.root_level->instruction_address, ctl.root_level->breakpoint_fd, ctl.root_level->total_triggered_times);
    ctl.root_level->triggered_times = 0;
  }
}

// we can perform multiple levels root cause analysis
// CHECK: whether current stage is finished
// CHECK: whether we need track more
bool RCATool::checkFinish() {
  bool isFinish = true;
  int all_rootcause = ctl.trace.num_site;
  int index = -1;
  for(int i=0; i<all_rootcause; i++) {
    if(ctl.trace.si[i].type == SITE_TYPE_ROOT_SUBLEVEL) {
      // remove repeated check
      for(int j=0; j<all_rootcause; j++) {
        if(ctl.trace.si[i].instruction_address 
            == ctl.trace.si[j].instruction_address
            && ctl.trace.si[j].type == SITE_TYPE_ROOT) {
          ctl.trace.si[i].type = SITE_TYPE_ROOT;
        }
      }

      if(ctl.trace.si[i].type == SITE_TYPE_ROOT_SUBLEVEL) {
        index = i;
      }
    }
    PRINT("%d/%d: thread %d, instruction %p, parent instruction %p, type %d, triggered times %d\n",
        i, all_rootcause, ctl.trace.si[i].tid,
        ctl.trace.si[i].instruction_address,
        (ctl.trace.si[i].parent==NULL ? 0 : ctl.trace.si[i].parent->instruction_address), ctl.trace.si[i].type, ctl.trace.si[i].total_triggered_times);
  }

#ifdef SUPPROT_MULTILEVEL
  if(index > 0) {
    isFinish = false;

    // recover every information to trace another level
    ctl.trace.cur = &ctl.trace.si[index];
    RCA_PRINF("track instruction %p for another level\n",
        ctl.trace.cur->instruction_address);

    ctl.reset();

    // recover current working set
    memcpy(ctl.callstack, ctl.trace.cur->callstack, ctl.trace.cur->level * sizeof(sFrame));
    ctl.cslevel = ctl.trace.cur->level;
    ctl.current_level = ctl.trace.cur->level_position;

    ctl.type = FAULTY_TYPE_PHONY;
    ctl.faultyip = (void*)ctl.trace.cur->callstack[0].regs[REG_RIP];
    // reset root level
    ctl.root_level = ctl.trace.cur->parent;

    ctl.sync_num = ctl.trace.cur->sync_num;
    for(int i=0; i<ctl.sync_num; i++) { 
      ctl.sync_event[i] = ctl.trace.cur->sync_event[i];
    }

    syncPTToCurPosition();
  }
#endif
  return isFinish;
}

void RCATool::syncPTToCurPosition() {
  thread_t* thread = getThreadByTid(ctl.trace.cur->tid);
  int thread_index = thread->index; 
  PerThreadDecoder* decoder = PTManager::getInstance().getDecoderByThread(thread_index);
  // reuse the PT data
  decoder->end_decode();
  decoder->start_decode();

  SyncEvent* upcomingevent = ctl.sync_event[0];
  SyncEvent* lastevent = thread->syncevents.getEntry(thread->syncevents.getEntriesNumb());
  //syncEvent* (SyncEventList::*recordFunc)(void*, thrSyncCmd, int) = &SyncEventList::recordSyncEvent;
  bool (xsync::*recordFunc)(void*, int*) = &xsync::peekSyncEvent;
  void* recordsync = (void*&)recordFunc;
  //void* recordsync = (void*)&SyncEventList::recordSyncEvent; // clang does not like
  int distance = lastevent - upcomingevent;
  RCA_PRINF("func is %p, distance %d, lastevent %p, upcomingevent %p\n", recordsync, distance, lastevent, upcomingevent);
  void* inst = decoder->pt_prev_instruction();
  while(distance > 0 && inst) {
    if(inst == recordsync) {
      distance--;
    }
    inst = decoder->pt_prev_instruction();
  } 
}

// PRINT out all root causes
void RCATool::printReport() {
  // output buffer
  char buf[256];
  PRINT("****************************************\nRoot Causes are:"); 
  PRINT("****************************************\nCalling Context (thread %d) is:", ctl.trace.cur->tid); 
  sFrame* calling_context = ctl.callstack; 
  for(int i=0; i<ctl.cslevel; i++) {
    void* addr = (void*)(calling_context[i].regs[REG_RIP] - PREV_INSTRUCTION_OFFSET);
    selfmap::getInstance().printCallStackByAddress(buf, i, addr);
  }

  int memoryaccesses = ctl.site.num_site;
  for(int im=0; im<memoryaccesses; im++) {
    void* faultyAddr = (void*)((intptr_t)ctl.site.info[im]->faultyAddr & ~0x3);
    if(faultyAddr == NULL) continue;

    CallStackInfo* write_context[5];
    int access_counter = 0;

    PRINT("\n****************************************\nMemory Access to %p is:", faultyAddr); 
    threadmap::aliveThreadIterator ti;
    for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
      thread_t* thread = ti.getThread();
      int index = thread->index;
      AllCallStackInfo* saved_callstack = &thread_access_buffer_[index];
      int num_callstack = saved_callstack->num_callstack;
      for(int j=0; j<num_callstack; j++) {
        if(saved_callstack->callstack[j].address == faultyAddr) {
          int level = saved_callstack->callstack[j].level;
          if(level != 0) {
            PRINT("===thread %d writes %lx at:\n", thread->tid, saved_callstack->callstack[j].value); 
            sFrame* real_callchain = saved_callstack->callstack[j].callstack;
            for(int k=0; k<level; k++) {
              void* addr = (void*)(real_callchain[k].regs[REG_RIP] - PREV_INSTRUCTION_OFFSET);
              selfmap::getInstance().printCallStackByAddress(buf, k, addr);
            }
            // record thread info
            write_context[access_counter++] = &saved_callstack->callstack[j];
          }
        }
      }
    }

    AnalysisHappenBeforeRelationship(ctl.trace.cur->tid, access_counter, write_context);

#ifdef SUPPROT_MULTILEVEL
    for(int i=0; i<access_counter; i++) {
      if(write_context[i] != NULL) {
        CallStackInfo* saved_callstack = write_context[i];
        // filter out interception functions
        int index = 0;
        while(selfmap::getInstance().isLibcLibrary((void*)saved_callstack->callstack[index].regs[REG_RIP])) ++index;
        if(selfmap::getInstance().isDoubleTakeLibrary((void*)saved_callstack->callstack[index].regs[REG_RIP])) continue;
        // get registers for memory addresses
        RCADisas::getInstance().saveMemoryAssignment(&ctl, ctl.site.info[im],
            ((thread_t*)saved_callstack->thread)->tid, saved_callstack);
      }
    }
#endif
  }
  
  PRINT("****************************************\n"); 
  if(memoryaccesses == 0) {
    PRINT("No more information is needed!!!\n\n****************************************\n"); 
  }

  // finish analysis for the current root cause
  ctl.trace.cur->type = SITE_TYPE_ROOT;
}

void RCATool::AnalysisHappenBeforeRelationship(int current_tid, int access_counter, CallStackInfo** write_context) {
  PRINT("****************************************\n"); 
  thread_t* last_thread = NULL;
  if(access_counter == 0) {
    PRINT("No Memory Access!!!"); 
  } else if(access_counter == 1) {
    if(((thread_t*)write_context[0]->thread)->tid == current_tid) {
      PRINT("The current thead writes the value to memory!!!"); 
    } else {
      CallStackInfo* saved_callstack = write_context[0];
      switch(CompareHappenBeforeBetweenCallStack(ctl.sync_num, ctl.sync_event, 
          saved_callstack->sync_num, saved_callstack->sync_event)) {
        case 0:
          PRINT("No happen before relationship found between thread %d and thread %d!!!", 
              current_tid, ((thread_t*)write_context[0]->thread)->tid); 
          break;
        case 1:
          // this is not possible
          PRINT("Can not determine thread order");
          break;
        case 2:
          PRINT("Thread %d happens before with crash thread!!!", ((thread_t*)write_context[0]->thread)->tid); 
          break;
      }
    }
  } else {
    // remove strong HB relationship thread
    for(int i=0; i<access_counter-1; i++) {
      if(write_context[i] != NULL) {
        CallStackInfo* last_callstack = write_context[i];
        for(int j=i+1; j<access_counter; j++) {
          if(write_context[j] != NULL) {
            CallStackInfo* new_callstack = write_context[j];

            // check HB Relationship
            CallStackInfo* result = CompareHappenBeforeBetweenThreads(last_callstack, new_callstack);
            if(result == new_callstack) {
              PRINT("Thread %d happens before thread %d!!!", ((thread_t*)write_context[i]->thread)->tid, ((thread_t*)write_context[j]->thread)->tid); 
              write_context[i] = NULL;
              break;
            }
            if(result == last_callstack) {
              PRINT("Thread %d happens before thread %d!!!", ((thread_t*)write_context[j]->thread)->tid, ((thread_t*)write_context[i]->thread)->tid); 
              write_context[j] = NULL;
            }
          }
        }
      }
    } 

    int total_last_write_thread = 0;
    for(int i=0; i<access_counter; i++) {
      if(write_context[i] != NULL) {
        last_thread = (thread_t*)write_context[i]->thread;
        total_last_write_thread++;
      }
    }

    if(total_last_write_thread == 1) { 
      if(last_thread->tid == current_tid) {
        PRINT("Last write thread is the current thread!!!"); 
      } else {
        PRINT("It might be a order violation between %d and %d!!!", 
            current_tid, last_thread->tid); 
      }
    } else {
      for(int i=0; i<access_counter; i++) {
        if(write_context[i] != NULL 
            && current_tid != ((thread_t*)write_context[i]->thread)->tid) {
          
          CallStackInfo* callstack = write_context[i];
          switch(CompareHappenBeforeBetweenCallStack(
                ctl.sync_num, ctl.sync_event, callstack->sync_num, callstack->sync_event)) {
            case 0:
              PRINT("It might be a order violation between %d and %d!!!", current_tid, ((thread_t*)callstack->thread)->tid);
              break;
            case 1:
              PRINT("Thread %d is before %d!!!", current_tid, ((thread_t*)callstack->thread)->tid);
              break;
            case 2:
              PRINT("Thread %d is before %d!!!",  ((thread_t*)callstack->thread)->tid, current_tid);
              break;
          }
        }
      }

    }
  }
  PRINT("****************************************\n"); 
}

// return the later one
CallStackInfo* RCATool::CompareHappenBeforeBetweenThreads(CallStackInfo* csinfo1, CallStackInfo* csinfo2) {
  CallStackInfo* csinfo = NULL;
  switch(CompareHappenBeforeBetweenCallStack(csinfo1->sync_num, csinfo1->sync_event, csinfo2->sync_num, csinfo2->sync_event)) {
    case 0:
      break;
    case 1:
      csinfo = csinfo2;
      break;
    case 2:
      csinfo = csinfo1;
      break;
  }
  return csinfo;
}

/* 
 * return value: 0, 1 ,2
 *  0: can not determine 
 *  1: the first callstack is before the second one 
 *  2: the second callstack is before the first one
 * */
int RCATool::CompareHappenBeforeBetweenCallStack(int sync_num1, SyncEvent** sync_event1, int sync_num2, SyncEvent** sync_event2) {
  int ret = 0;

  // compare lock order
  for(int i=1; i<sync_num1; i++) {
    SyncEvent* event1 = sync_event1[i];
    for(int j=1; j<sync_num2; j++) {
      SyncEvent* event2 = sync_event2[j];
      ret |= CompareHappenBeforeBetweenEvent(event1, event2);
    }
  }

  // if sync1 is not before sync2
  if (ret == 0) {
    for(int i=1; i<sync_num2; i++) {
      SyncEvent* event2 = sync_event2[i];
      for(int j=1; j<sync_num1; j++) {
        SyncEvent* event1 = sync_event1[j];
        ret |= CompareHappenBeforeBetweenEvent(event2, event1);
      }
    }

    // change return value
    if(ret == 1) {
      ret = 2;
    }
  }

  // if we can not find happen before via synchronization
  if(ret == 0) {
    SyncEvent* upcomingevent1 = sync_event1[0];
    SyncEvent* upcomingevent2 = sync_event2[0];
    RCA_PRINF("upcomingevent1 is %p, upcomingevent2 is %p\n", upcomingevent1, upcomingevent2);
    // check order
    if(CompareHappenBeforeBetweenEvent(upcomingevent1, upcomingevent2-1)) {
      ret = 1;
    }
    if(CompareHappenBeforeBetweenEvent(upcomingevent2, upcomingevent1-1)) {
      ret = 2;
    }
  }

  return ret;
}

/* 
 * return value: 0, 1
 *  0: can not determine 
 *  1: sync_event1 happens before sync_event2
 * */
int RCATool::CompareHappenBeforeBetweenEvent(SyncEvent* sync_event1, SyncEvent* sync_event2) {
  int ret = 0;
  // check whether they are the same lock
  if(sync_event1->eventlist == sync_event2->eventlist) {
    SyncEvent* it_event = sync_event1->next;
    while(it_event!=NULL) {
      if(it_event == sync_event2) {
        ret = 1;
        break;
      }
      it_event = it_event->next;
    }
  } else {
    // traverse per-thread list to find the event in the same lock
    // check the event order in per-thread list in another thread
    SyncEvent* it_sync_event1 = sync_event1;
    while(it_sync_event1->thread != NULL) {
      SyncEvent* it_event = it_sync_event1->next;
      while(it_event!=NULL) {
        if(it_event->thread == sync_event2->thread) {
          if(it_event <= sync_event2) {
            ret = 1;
          }
          break;
        }
        it_event = it_event->next;
      }

      // if we find HB
      if(ret != 0) break;

      it_sync_event1++;
    }
  }
  return ret;
}

void RCATool::installAllBreakPoint() {
  int num_inst = ctl.site.num_site;
  if(num_inst > xdefines::MAX_WATCHPOINTS) {
#ifdef HAVE_SOFTWARE_BREAKPOINT
    for(int i=0; i<num_inst; i++) {
      if(ctl.site.info[i]->faultyAddr == NULL) {
        pid_t tid = ctl.site.info[i]->tid;
        sw_breakpoint.SetBreakpoint(ctl.site.info[i]->instruction_address, tid, NULL);
        RCA_PRINF("install %d/%d software breakpoint at %p for thread %d\n", i, num_inst, ctl.site.info[i]->instruction_address, tid);
      }
    }
#else 
    PRINT("Faulty addresses are larger than maximum watchpoints!!! Please use software breakpoint.\n");
#endif
  } else {
    for(int i=0; i<num_inst; i++) {
      if(ctl.site.info[i]->faultyAddr == NULL) {
        pid_t tid = ctl.site.info[i]->tid;
        ctl.site.info[i]->breakpoint_fd = installBreakPoint(ctl.site.info[i]->instruction_address, tid);
        RCA_PRINF("install %d/%d hardware breakpoint at %p, fd %d for thread %d\n", i, num_inst, ctl.site.info[i]->instruction_address, ctl.site.info[i]->breakpoint_fd, tid);
      }
    }
  }
}

void RCATool::disableWatchPoint() {
  int num_addr = ctl.site.num_site;
  for(int i=0; i<num_addr; i++) {
    // align up the address, otherwise perf can not install
    void* addr = (void*)((intptr_t)ctl.site.info[i]->faultyAddr & ~0x3);
    if(addr == NULL) continue;

    watchpoint::faultyObject *object = watchpoint::getInstance().findFaultyObjectByStartAddress(addr);
    if (object != NULL
        && object->objtype==OBJECT_TYPE_DATATRACE
        && object->isMonitored==1) {
      watchpoint::getInstance().disableWatchpointByObject(object, true);
      RCA_PRINF("disable %d/%d hardware watchpoint at %p\n", i, num_addr, addr);
    } 
  }
}

void RCATool::disableBreakPoint() {
  int num_inst = ctl.site.num_site;
  if(num_inst > xdefines::MAX_WATCHPOINTS) {
#ifdef HAVE_SOFTWARE_BREAKPOINT
    for(int i=0; i<num_inst; i++) {
      if(ctl.site.info[i]->faultyAddr == NULL) {
        sw_breakpoint.RemoveBreakpoint(ctl.site.info[i]->instruction_address);
      }
    }
#endif
  } else {
    for(int i=0; i<num_inst; i++) {
      if(ctl.site.info[i]->faultyAddr == NULL) {
        int fd = ctl.site.info[i]->breakpoint_fd;
        if(fd > 2) {
          if(Real::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            RCA_PRWRN("Failed to disable perf event: %s\n", strerror(errno));
          }
          Real::close(fd);
          RCA_PRINF("disable %d/%d breakpoint at %d\n", i, num_inst, fd);
          ctl.site.info[i]->breakpoint_fd = 0;
        }
      }
    }
  }
}

int RCATool::installBreakPoint(void* address, pid_t pid) {
  // install breakpoint on each instruction 
#if 0
  struct perf_event_attr pe = {
    .type = PERF_TYPE_BREAKPOINT,
    .size = sizeof(struct perf_event_attr),
    .bp_type = HW_BREAKPOINT_X,
    .bp_len = sizeof(long),
    .bp_addr = (uintptr_t)address,
    .disabled = 1,
    .sample_period = 1, };
#endif
  struct perf_event_attr pe;
  memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_BREAKPOINT;
  pe.size = sizeof(pe);
  pe.bp_type = HW_BREAKPOINT_X;
  pe.bp_len = sizeof(long);
  pe.bp_addr = (uintptr_t)address;
  pe.disabled = 1;
  pe.sample_period = 1;

  int perf_fd = perf_event_open(&pe, pid, -1, -1, 0);
  if(perf_fd == -1) {
    RCA_PRWRN("Failed to open perf event file: pid %d,  %s\n", pid, strerror(errno));
    exit(-1);
  }
  if(Real::fcntl(perf_fd, F_SETFL, Real::fcntl(perf_fd, F_GETFL, 0) | O_ASYNC) == -1) {
    RCA_PRWRN("Failed to set perf event file to ASYNC mode: %s\n", strerror(errno));
    exit(-1);
  }
  if(Real::fcntl(perf_fd, F_SETSIG, SIGUSR2) == -1) {
    RCA_PRWRN("Failed to set perf event file's async signal: %s\n", strerror(errno));
    exit(-1);
  }
  struct f_owner_ex owner = {F_OWNER_TID, pid};
  if(Real::fcntl(perf_fd, F_SETOWN_EX, &owner) == -1) {
    RCA_PRWRN("Failed to set the owner of the perf event file: %s\n", strerror(errno));
    exit(-1);
  }
  if(Real::ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
    RCA_PRWRN("Failed to enable perf event: %s\n", strerror(errno));
    exit(-1);
  }

  return perf_fd;
}

#ifdef HAVE_SOFTWARE_BREAKPOINT

breakpoint_t* RCATool::FindBreakpointByAddress(void* address) {
  return sw_breakpoint.FindBreakpointByAddress(address);
}

bool RCATool::filterContext(void* inst, void* real_inst, int signum, siginfo_t* siginfo, void* context) {

  if(selfmap::getInstance().isLibcLibrary(inst)) {
    int frames;
    int index;

    sFrame callstack[64];
    //void* callstack_array[256];
    xthread::disableCheck();
    //frames = backtrace(callstack_array, 256);
    frames = my_backtrace(callstack, 64);
    xthread::enableCheck();

    //while(callstack_array[index] != inst) index++;
    for(index=0; index<frames; index++) {
      if(callstack[index].regs[REG_RIP] == (intptr_t)inst){
        break;
      }
    }
    // rule out our library
    // TODO compare current callstack with SEGV callstack
    while(selfmap::getInstance().isLibcLibrary((void*)callstack[index].regs[REG_RIP])) ++index;
    if(selfmap::getInstance().isDoubleTakeLibrary((void*)callstack[index].regs[REG_RIP])) return true;
  }

  //some assignments are conditional
  //if(!RCATool::getInstance().IsExecutedInstruction(real_inst, context)){
  //  return true;
  //}
  return false;
}

// This is used for checking multiple assignments and collect context data
void RCATool::SoftwareBreakpointHandler(breakpoint_t* bp, int signum, siginfo_t* siginfo, void* context) {
  ucontext_t* trapcontext = (ucontext_t*)context;
  char* inst = (char*)(trapcontext->uc_mcontext.gregs[REG_RIP]); 

  if(filterContext(inst, bp->inst_data, signum, siginfo, context)) {
    return;
  }

  // ip is the next instruction of INT 3, it should be subtracted by 1 
  --inst; 
  //RCA_PRINF("software breakpoint on %p\n", inst);
  trackctl* ctl = RCATool::getInstance().getTrackCtl();
  int num_inst = ctl->site.num_site;
  for(int i=0; i<num_inst; i++) {
    if(ctl->site.info[i]->instruction_address == inst) {
      if(ctl->cmd == TRACK_CMD_CONTEXT) {
        ctl->site.info[i]->total_triggered_times++;
        // copy value of register from REG_R8 to REG_CR2
        for(int reg=REG_R8; reg<=REG_CR2; reg++) {
          ctl->site.context[i].regs[reg] = ((ucontext_t *)trapcontext)->uc_mcontext.gregs[reg];
        }
        //memcpy(ctl->sInst.context[i].regs, ((ucontext_t *)trapcontext)->uc_mcontext.gregs, sizeof(gregset_t)); 
      }
    }
  }
}
#endif

// Handling the signal SIGUSR2
void RCATool::sigusr2Handler(int signum, siginfo_t* siginfo, void* context) {
  ucontext_t* trapcontext = (ucontext_t*)context;
  size_t* inst = (size_t*)trapcontext->uc_mcontext.gregs[REG_RIP]; // address of access 

  //RCA_PRINF("breakpoint on %p\n", inst);
  if(RCATool::getInstance().filterContext(inst, inst, signum, siginfo, context)) {
    return;
  }

  trackctl* ctl = RCATool::getInstance().getTrackCtl();

  if(ctl->root_level != NULL && ctl->root_level->instruction_address == inst) {
    ++ctl->root_level->triggered_times; 
    RCA_PRINF("ROOT_CAUSE: triggered_times is %d, total_triggered_times %d\n", 
        ctl->root_level->triggered_times, ctl->root_level->total_triggered_times);
    if(ctl->root_level->triggered_times == ctl->root_level->total_triggered_times) {
      // disable pt first
      PTManager::getInstance().pt_DisableAll();
      // stop all thread
      xrun::getInstance().stopAllThreads();
#ifdef GET_REPLAY_RUNTIME
      unsigned long current_time = getCurrentTime();
      PRINT("current time is %ld", current_time);
#endif
      if(!xthread::getInstance().checkSkipStatus()){
        RCA_PRINF("STOP: triggered_times is %d, total_triggered_times is %d\n", 
            ctl->root_level->triggered_times, ctl->root_level->total_triggered_times);
    
        xthread::disableCheck();
        // continue to analyze
        RCATool::getInstance().resumeAnalysis();
        xthread::enableCheck();

        // increment replay times
        global_addrbSegvNumber();
        // replay 
        jumpToFunction((ucontext_t*)context, (unsigned long)xmemory::getInstance().crashHookForRollback);
      }
    }
  } else {
    int num_inst = ctl->site.num_site;
    for(int i=0; i<num_inst; i++) {
      if(ctl->site.info[i]->instruction_address == inst) {
        if(ctl->cmd == TRACK_CMD_CONTEXT) {
          ctl->site.info[i]->total_triggered_times++;
          // copy value of register from REG_R8 to REG_CR2
          for(int reg=REG_R8; reg<=REG_CR2; reg++) {
            ctl->site.context[i].regs[reg] = ((ucontext_t *)trapcontext)->uc_mcontext.gregs[reg];
          }
          //memcpy(ctl->sInst.context[i].regs, ((ucontext_t *)trapcontext)->uc_mcontext.gregs, sizeof(gregset_t)); 
        }
      }
    }
  }
}

// @brief Install a handler for SIGUSR2 signals.
void RCATool::installSigUsr2Handler() {

  struct sigaction sigusr2;
  // We don't want to receive SIGUSR2 again when a thread is inside signal handler.
  sigemptyset(&sigusr2.sa_mask);
  sigaddset(&sigusr2.sa_mask, SIGUSR2);

  /**
    Some parameters used here:
    SA_RESTART: Provide behaviour compatible with BSD signal
      semantics by making certain system calls restartable across signals.
    SA_SIGINFO: The signal handler takes 3 arguments, not one. In this case, sa_sigaction
      should be set instead of sa_handler. 
    So, we can acquire the user context inside the signal handler
  */
  //sigusr2.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
  sigusr2.sa_flags = SA_SIGINFO | SA_RESTART;
  sigusr2.sa_sigaction = RCATool::sigusr2Handler;
  if(Real::sigaction(SIGUSR2, &sigusr2, NULL) == -1) {
    RCA_PRWRN("setting signal handler SIGUSR2 failed.\n");
    exit(-1);
  }
}

void RCATool::SaveCallStackAndValue(int thread, void* addr, void* memoryaddr, long value, int level, sFrame* callstack, void* current_ctx) {
  if(ctl.root_level != NULL && ctl.root_level->total_triggered_times != 0 
      && ctl.root_level->total_triggered_times <= ctl.root_level->triggered_times) {
    return;
  }

  // find callstack and save changed value
  AllCallStackInfo* allcsinfo = &thread_access_buffer_[thread];
  CallStackInfo* csinfo = NULL;

  for(int i=0; i<allcsinfo->num_callstack; i++) {
    if(allcsinfo->callstack[i].address == memoryaddr) {
      csinfo = &allcsinfo->callstack[i];
    } 
  }
  if(csinfo == NULL) {
    csinfo = &allcsinfo->callstack[allcsinfo->num_callstack++];
  }

  csinfo->thread = current;
  csinfo->address = memoryaddr;
  csinfo->value = value;

  // save callsite
  int i = 0;
  while(callstack[i].regs[REG_RIP] != (intptr_t)addr) i++;

  // save current context
  for(int reg=REG_R8; reg<=REG_CR2; reg++) {
    callstack[i].regs[reg] = ((ucontext_t *)current_ctx)->uc_mcontext.gregs[reg];
  }

  // save callstack
  csinfo->level = level-i;
  for(int j=0; j<csinfo->level; j++,i++) {
    csinfo->callstack[j] = callstack[i];
  }

  SaveCurrentThreadSync(&(csinfo->sync_num), csinfo->sync_event);
}

void RCATool::SaveCurrentThreadSync(int* sync_num, SyncEvent** sync_event) {
  // save position event
  SyncEvent* upcomingevent = current->syncevents.firstIterEntry();
  *sync_num = 1;
  sync_event[0] = upcomingevent;
  // save sync event
  int lockcounter = current->lockcounter;
  if(lockcounter>0) {
    SyncEvent* startevent = current->syncevents.getEntry(0);
    SyncEvent* itevent = upcomingevent-1;

    while(itevent >= startevent
        && lockcounter>0) {
      SyncEventList* synclist = (SyncEventList*)itevent->eventlist; 
      if(synclist->getCurrentSyncEvent() == itevent) {
        //RCA_PRINF("record %d synchronization: eventlist %p, thread %p, syncvar %p, cmd %d\n", 
        //    lockcounter, itevent->eventlist, itevent->thread, itevent->syncvar, itevent->synccmd);
        // save event
        sync_event[*sync_num] = itevent;

        (*sync_num)++;
        lockcounter--; 
      }
      itevent--;
    }
  }
}

