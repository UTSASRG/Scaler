/*
 * @file   watchpoint.cpp
 * @brief  Including the implemenation of watch point handler.
 *         Since we have to call functions in other header files, then it is
 *         impossible to put this function into the header file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "watchpoint.hh"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

#include "log.hh"
#include "memtrack.hh"
#include "selfmap.hh"
#include "xdefines.hh"
#include "threadmap.hh"
#include "rca_tool.hh"
#include "backtrack.hh"
#include "xthread.hh"

#ifdef HAVE_SOFTWARE_BREAKPOINT

#include "vm_emulator.h"

#endif

long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd,
                     unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

#if 0
pid_t gettid() {
  return syscall(__NR_gettid);
}
#endif

bool watchpoint::addWatchpoint(void *addr, size_t value, faultyObjectType objtype,
                               void *objectstart, size_t objectsize) {
    bool hasWatchpoint = true;

#ifndef EVALUATING_PERF
    if (objtype == OBJECT_TYPE_OVERFLOW) {
        PRINT("iReplayer: Buffer overflow at address %p with value 0x%lx. size %lx start %p\n",
              addr, value, objectsize, objectstart);
        // PRINT("DoubleTake: Buffer overflow at address %p with value 0x%lx. \n", addr, value);
    } else if (objtype == OBJECT_TYPE_USEAFTERFREE) {
        // assert(objtype == OBJECT_TYPE_USEAFTERFREE);
        PRINT("iReplayer: Use-after-free error detected at address %p.", addr);
    }
#endif

    if (_numWatchpoints < xdefines::MAX_WATCHPOINTS) {
        // Record watch point information
        _wp[_numWatchpoints].faultyaddr = addr;
        _wp[_numWatchpoints].objectstart = objectstart;
        _wp[_numWatchpoints].objtype = objtype;
        //  _wp[_numWatchpoints].objectsize = objectsize;
        _wp[_numWatchpoints].faultyvalue = value;
        _wp[_numWatchpoints].currentvalue = value;
        _numWatchpoints++;
    } else {
        hasWatchpoint = false;
    }

    // Add it to memtrack too.
    if (objtype != OBJECT_TYPE_WATCHONLY
        && objtype != OBJECT_TYPE_DATATRACE)
        memtrack::getInstance().insert(objectstart, objectsize, objtype);

    return !hasWatchpoint;
}

// find object by address
watchpoint::faultyObject *watchpoint::findFaultyObjectByStartAddress(void *startaddress) {

    for (int i = 0; i < _numWatchpoints; i++) {
        if (_wp[i].objectstart == startaddress) {
            return &_wp[i];
        }
    }

    return NULL;
}

// find object by address
watchpoint::faultyObject *watchpoint::findFaultyObjectByRange(MemAccess *mem) {
    intptr_t start = (intptr_t) mem->addr;
    intptr_t end = start + mem->length;
    for (int i = 0; i < _numWatchpoints; i++) {
        if ((intptr_t) _wp[i].objectstart >= start
            && (intptr_t) _wp[i].objectstart < end) {
            return &_wp[i];
        }
    }

    return NULL;
}

bool watchpoint::findFaultyObject(faultyObject **object) {
    int trigPoints = 0;

//  PRINT("findFaultyObject: _numWatchpoints %d\n", _numWatchpoints);
    for (int i = 0; i < _numWatchpoints; i++) {
        //unsigned long value = *((unsigned long*)_wp[i].faultyaddr);
        unsigned long value = *((unsigned int *) _wp[i].faultyaddr);
#ifndef EVALUATING_PERF
        //PRINT("DoubleTake: checking %d point: address %p currentvalue %lx value %lx\n", i, _wp[i].faultyaddr, _wp[i].currentvalue, value);
#endif
        // Check whether now overflow actually happens
        if (value != _wp[i].currentvalue || _numWatchpoints == 1) {
            //				PRINT("WARNING: we %d points, currentvalue %lx value %lx\n", trigPoints,
            //_wp[i].currentvalue, value);
            _wp[i].currentvalue = value;
            /*
            if(_wp[i].objtype == OBJECT_TYPE_DATATRACE
                && value != _wp[i].faultyvalue)
              continue;
              */
            *object = &_wp[i];
            trigPoints++;
        }
    }

    if (trigPoints > 1) {
        PRINT("WARNING: we have %d watchpoints triggered, only one watchpoint allowed.\n", trigPoints);
    }

    return (trigPoints != 0);
}

bool watchpoint::hasToRollback() { return _numWatchpoints > 0; }

void watchpoint::installWatchpoints() {
    struct sigaction trap_action;

    //return;
    //  PRINT("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);
    // We don't need to setup watchpoints if it is 0.
    if (_numWatchpoints == 0) {
        return;
    }

    // Now we are setting a trap handler for myself.
    trap_action.sa_sigaction = watchpoint::trapHandler;
    trap_action.sa_flags = SA_SIGINFO | SA_RESTART;
    //trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    Real::sigaction(SIGTRAP, &trap_action, NULL);

    int perffd;
    //PRINT("Install watchpoints: _numWatchpoints %d\n", _numWatchpoints);
    // Get the initial value of different watchpoints.
    threadmap::aliveThreadIterator ti;
    for (ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
        thread_t *thread = ti.getThread();
        if (thread->action != E_THREAD_ACTION_EXIT)
            for (int i = 0; i < _numWatchpoints; i++) {
                // PRINT("Watchpoint %d: addr %p. _numWatchpoints %d\n", i, _wp[i].faultyaddr, _numWatchpoints);
                // PRINT("Watchpoint %d: addr %p\n", i, _wp[i].faultyaddr);
                // Update the values of those faulty address so that
                // we can compare those values to find out which watchpoint
                // are accessed since we don't want to check the debug status register
                _wp[i].index = i;
                _wp[i].currentvalue = *((unsigned int *) _wp[i].faultyaddr);
                //_wp[i].currentvalue = *((unsigned long*)_wp[i].faultyaddr);

                // install this watch point.
                perffd = install_watchpoint((uintptr_t) _wp[i].faultyaddr, thread->tid, SIGTRAP, -1);
                thread->wpfd[i] = perffd;
                PRINF("Threadid %d, perf %d, Watchpoint %d: addr %p done\n", thread->tid, perffd, i, _wp[i].faultyaddr);
                //Now we can start those watchpoints.

#ifdef DETECT_USAGE_AFTER_FREE
                if (_wp[i].objtype != OBJECT_TYPE_USEAFTERFREE) {
#endif
                enable_watchpoints(perffd);
                _wp[i].isMonitored = 1;
                // We actually don't care about what content.
#ifdef DETECT_USAGE_AFTER_FREE
                }
#endif
            }
        //PRINT("Threadid %d, add watchpoint done\n", thread->tid);
    }
}

int watchpoint::install_watchpoint(uintptr_t address, pid_t pid, int sig, int group) {
#if 1
    // Perf event settings
    struct perf_event_attr pe;
    pe.type = PERF_TYPE_BREAKPOINT;
    pe.size = sizeof(struct perf_event_attr);
    pe.bp_type = HW_BREAKPOINT_W;
    pe.bp_len = HW_BREAKPOINT_LEN_4;
    pe.bp_addr = (uintptr_t) address;
    pe.sample_period = 1;
#else
    struct perf_event_attr pe;
    PRINT("perf_event_attr pe %p, size %zu", &pe, sizeof(pe));
    memset(&pe, 0, sizeof(pe));
    pe.type = PERF_TYPE_BREAKPOINT;
    pe.size = sizeof(pe);
    pe.bp_type = HW_BREAKPOINT_W;
    pe.bp_len = HW_BREAKPOINT_LEN_4;
    pe.bp_addr = (uintptr_t)address;
    pe.disabled = 1;
    pe.sample_period = 1;
#endif

    // Create the perf_event for this thread on all CPUs with no event group, use pid instead of 0.
    int perf_fd = perf_event_open(&pe, pid, -1, group, 0);
    if (perf_fd == -1) {
        PRINT("Failed to open perf event file: pid %d,  %s\n", pid, strerror(errno));
        abort();
    }

    // PRINT("Install watchpoint. perf_fd %d group %d\n", perf_fd, group);

    // Set the perf_event file to async mode
    if (Real::fcntl(perf_fd, F_SETFL, Real::fcntl(perf_fd, F_GETFL, 0) | O_ASYNC) == -1) {
        PRINT("Failed to set perf event file to ASYNC mode: %s\n", strerror(errno));
        abort();
    }

    // Tell the file to send a SIGTRAP when an event occurs
    if (Real::fcntl(perf_fd, F_SETSIG, sig) == -1) {
        PRINT("Failed to set perf event file's async signal: %s\n", strerror(errno));
        abort();
    }

    // Deliver the signal to this thread
    //   if(Real::fcntl(perf_fd, F_SETOWN, syscall(__NR_gettid)) == -1) {
    //if(Real::fcntl(perf_fd, F_SETOWN, getpid()) == -1) {
    if (Real::fcntl(perf_fd, F_SETOWN, pid) == -1) {
        fprintf(stderr, "Failed to set the owner of the perf event file: %s\n", strerror(errno));
        abort();
    }

    return perf_fd;
}

void watchpoint::enableWatchpointByObject(faultyObject *object) {
    // Get the initial value of different watchpoints.
    threadmap::aliveThreadIterator ti;
    for (ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
        thread_t *thread = ti.getThread();
        if (thread->action != E_THREAD_ACTION_EXIT) {
            object->currentvalue = *((unsigned int *) object->faultyaddr);
            //object->currentvalue = *((unsigned long*)object->faultyaddr);
            enable_watchpoints(thread->wpfd[object->index]);
            object->isMonitored = 1;
        }
    }
}

void watchpoint::disableWatchpointByObject(faultyObject *object, bool isClose) {

    // reset object value
    object->currentvalue = *((unsigned int *) object->faultyaddr);
    //object->currentvalue = *((unsigned long*)object->faultyaddr);
    object->isMonitored = 0;

    // remove  watchpoints for each thread.
    threadmap::aliveThreadIterator ti;
    for (ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
        thread_t *thread = ti.getThread();
        if (thread->action != E_THREAD_ACTION_EXIT) {
            disable_watchpoint(thread->wpfd[object->index]);
            if (isClose) {
                Real::close(thread->wpfd[object->index]);
            }
        }
    }

    // decrease watchpoint number
    // and fill the hole
    _wp[object->index] = _wp[_numWatchpoints - 1];
    --_numWatchpoints;
}

void watchpoint::enable_watchpoints(int fd) {
    // Start the event
    if (Real::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
        fprintf(stderr, "Failed to enable perf event: %s\n", strerror(errno));
        abort();
    }
}

void watchpoint::disable_watchpoint(int fd) {
    // Start the event
    if (Real::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
        fprintf(stderr, "Failed to disable perf event: %s\n", strerror(errno));
        abort();
    }
}

bool watchpoint::checkAndSaveCallsite(faultyObject *object, int depth, void **callsite) {
    return object->faultySite.saveAndCheck(depth, callsite);
}

// Handle those traps on watchpoints now.
void watchpoint::trapHandler(int sig, siginfo_t *siginfo, void *context) {
    ucontext_t *trapcontext = (ucontext_t *) context;
    size_t *addr = (size_t *) trapcontext->uc_mcontext.gregs[REG_RIP]; // address of access

#ifdef HAVE_SOFTWARE_BREAKPOINT
    unsigned char *prev_addr = (unsigned char *) (trapcontext->uc_mcontext.gregs[REG_RIP] - 1);
    if (*prev_addr == 0xCC) {
        // this could be software breakpoint
        breakpoint_t *bp = RCATool::getInstance().FindBreakpointByAddress(prev_addr);
        if (bp != NULL) {
            void *nextinst = NULL;

            pid_t current_thread = current->tid;
            if (bp->thread_id == current_thread) {
                RCATool::getInstance().SoftwareBreakpointHandler(bp, sig, siginfo, context);
            }
            // execute instruction on breakpoint
            nextinst = VmEmulator::getInstance().ExecuteInstruction(bp, context);
            // move to next instruction
            trapcontext->uc_mcontext.gregs[REG_RIP] = (intptr_t) nextinst;
            return;
        }
    }
#endif

//	return;
    // Find faulty object.
    MemAccess mem;
    RCATool::getInstance().getMemoryAddressByPrevInstruction(addr, context, &mem);
    void *adjust_addr = (void *) ((intptr_t) mem.addr & ~0x3);
    PRINF(" inside the trap handler, address %p with value %lx, memory address is %p (%p), value %x, length %zu\n",
          addr, *((unsigned long *) addr), mem.addr, adjust_addr, adjust_addr ? *(unsigned int *) adjust_addr : 0,
          mem.length);
    mem.addr = adjust_addr;

    faultyObject *object;
    // If it is a read, we only care about this if it is use-after-free error
    if (!(object = watchpoint::getInstance().findFaultyObjectByRange(&mem))) {
        PRINF("Can't find faulty object!!!!\n");
        return;
    }

    // PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));
    // Check whether this trap is caused by libireplayer library.
    // If yes, then we don't care it since libireplayer can fill the canaries.
    if (selfmap::getInstance().isDoubleTakeLibrary(addr) || selfmap::getInstance().isDlLibrary(addr)) {
        return;
    }

    //record the bug is found in rollback
    g_bugReproduced = true;
    //PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));

    //  PRINF("CAPTURING write at %p: ip %lx. signal pointer %p, code %d. \n", addr,
    // trapcontext->uc_mcontext.gregs[REG_RIP], siginfo->si_ptr, siginfo->si_code);
    faultyObjectType faultType;
    if (object->objtype != OBJECT_TYPE_WATCHONLY && object->objtype != OBJECT_TYPE_DATATRACE) {
        faultType = memtrack::getInstance().getFaultType(object->objectstart, object->faultyaddr);
        if (faultType == OBJECT_TYPE_NO_ERROR) {
            return;
        }
    } else {
        //PRINT("\nWatch %p a memory access on %p (value %lx) with call stack:\n", addr, object->faultyaddr, *((unsigned long *)object->faultyaddr));
        //selfmap::getInstance().printCallStack();
        bool disable = current->disablecheck;
        if (!disable) {
            xthread::disableCheck();
        }
        sFrame callstack[32];
        memset(callstack, 0, sizeof(callstack));
        int fnum = my_backtrace(callstack, 31);
        RCATool::getInstance().SaveCallStackAndValue(current->index, addr, object->faultyaddr,
                                                     *((unsigned int *) object->faultyaddr), fnum, callstack, context);
        if (!disable) {
            xthread::enableCheck();
        }
        return;
    }
//  PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));

    // Check whether this callsite is the same as the previous callsite.
    void *callsites[xdefines::CALLSITE_MAXIMUM_LENGTH];
//  PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));
    int depth = selfmap::getCallStack((void **) &callsites);

//  PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));
    // If current callsite is the same as the previous one, we do not want to report again.
    if (watchpoint::getInstance().checkAndSaveCallsite(object, depth, (void **) &callsites)) {
        return;
    }
//  PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));

    // Now we should check whether objectstart is existing or not.
    if (faultType == OBJECT_TYPE_OVERFLOW) {
        PRINT("\nCaught a heap overflow at %p. Current call stack:\n", object->faultyaddr);
    } else if (faultType == OBJECT_TYPE_USEAFTERFREE) {
        PRINT("\nCaught a use-after-free error at %p. Current call stack:\n", object->faultyaddr);
    }
    selfmap::getInstance().printCallStack(depth, (void **) &callsites);
//  PRINT("LINE %d: inside the trap handler, address %p with value %lx\n", __LINE__, addr, *((unsigned long *)addr));

    // Check its allocation or deallocation inf
    if (object->objectstart != NULL) {
        // Now we should check memtrack status.
        memtrack::getInstance().print(object->objectstart, faultType);
    }
}
