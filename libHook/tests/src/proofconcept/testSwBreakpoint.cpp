#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <assert.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <errno.h>
#include <sys/user.h>

#include <libunwind.h>
#include <libunwind-x86_64.h>
#include <libunwind-ptrace.h>

using namespace std;

#define STR_MAX 1000
#define DEBUGGEE "/media/umass/datasystem/steven/Scaler/cmake-build-debug/libScalerHook/tests/libScalerHook-proof-ManualBreakpoint"

int PrintFileAndLine(const char *debugSymbol, unw_word_t addr)  //call this function once per stack frame
{
    char buffer[STR_MAX] = {};
    sprintf(buffer, "/usr/bin/addr2line -C -e %s -f -i %lx", debugSymbol, addr); //I probably copied this from somewhere
    FILE *f = popen(buffer, "r");  //open process


    fgets(buffer, sizeof(buffer), f);
    printf("function:%s  ", buffer);
    fgets(buffer, sizeof(buffer), f);
    printf("file/line:%s******\n", buffer);

    pclose(f);
}

//void Getbacktrace(int thetid) {
//    unw_cursor_t cursor;
//    unw_word_t ip;
//    unw_addr_space_t as;
//    struct UPT_info *ui = NULL;
//
//    as = unw_create_addr_space(&_UPT_accessors, 0);
//    ui = static_cast<struct UPT_info *>(_UPT_create(thetid));
//
//    int rc = unw_init_remote(&cursor, as, ui);
//    if (rc != 0) {
//        if (rc == UNW_EINVAL) {
//            printf("unw_init_remote: UNW_EINVAL");
//        } else if (rc == UNW_EUNSPEC) {
//            printf("unw_init_remote: UNW_EUNSPEC");
//        } else if (rc == UNW_EBADREG) {
//            printf("unw_init_remote: UNW_EBADREG");
//        } else {
//            printf("unw_init_remote: UNKNOWN");
//        }
//    }
//
//    while (unw_step(&cursor) > 0) {
//        unw_word_t offset, pc;
//        unw_get_reg(&cursor, UNW_REG_IP, &pc);
//
//        char buffer[STR_MAX] = {};
//        if (unw_get_proc_name(&cursor, buffer, sizeof(buffer), &offset) == 0) //get mangled function names
//            printf("%s\n", buffer);
//
//        PrintFileAndLine(DEBUGGEE, pc);  //use addr2line
//    }
//
//    _UPT_destroy(ui);
//    unw_destroy_addr_space(as);
//}

int main(int argc, char *argv[]) {
    pid_t pid = 0;
    switch (pid = fork())  //spqwn a process
    {
        case 0://child process
        {
            int y = execlp(DEBUGGEE, 0);
            break;
        }
        case -1:
            printf("error spawning process\n");
            exit(-1);
            break;
            //parent continues execution
    }


    printf("child created:%u\n", pid);
    int status = 0;
    ptrace(PTRACE_ATTACH, pid, NULL);
    printf("error %u\n", errno);   //lets attach the process

    while (1) {
        pid_t tid = wait(&status);
        if (WIFSTOPPED(status)) {
            printf("child has stopped \n");

            siginfo_t siginfo = {};
            ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo);
            printf("signal caught:%u\n", siginfo.si_signo);

            //get register context
            user uregs = {};
            ptrace(PTRACE_GETREGS, pid, NULL, &uregs);
            printf("rax=%x rbx=%x rcx=%x rdx=%x rip=%x \n", uregs.regs.rax, uregs.regs.rbx, uregs.regs.rcx,
                   uregs.regs.rdx, uregs.regs.rip);
            printf("cs=%x ss=%x ds=%x\n", uregs.regs.cs, uregs.regs.ss, uregs.regs.ds);

            printf("some btes at the current RIP:");
            for (unsigned long i = uregs.regs.rip - 2; i < uregs.regs.rip + 4; ++i) {
                unsigned char s = ptrace(PTRACE_PEEKTEXT, pid, i, 0);
                unsigned int j = s;
                printf("%x ", j);
            }
            printf("\n");

            //Getbacktrace(tid);

            /*
             * long ptrace(enum __ptrace_request request, pid_t pid,
                   void *addr, void *data);

             * PTRACE_CONT
                          Restart  the  stopped tracee process.  If data is nonzero, it is interpreted as the number of a signal to be delivered to the tracee;
                          otherwise, no signal is delivered.  Thus, for example, the tracer can control whether a signal sent to the  tracee  is  delivered  or
                          not.  (addr is ignored.)
            */

            //note that you may want to swallow up TRAP signal (5) as it is done in QT (GDB)
            if (5 != siginfo.si_signo)
                ptrace(PTRACE_CONT, pid, NULL, siginfo.si_signo);  //send the signal back to the tracee
            else
                ptrace(PTRACE_CONT, pid, NULL, NULL);  //send the signal back to the tracee
        }
    }

    return 0;
}

