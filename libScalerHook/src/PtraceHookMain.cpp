#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <util/tool/Logging.h>
#include <util/hook/install.h>
#include <thread>
#include <sys/prctl.h>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>
#include <string.h>

void run_target(const char *programname) {
    DBG_LOGS("target started. will run '%s'", programname);
    //Set dumpable so that we can read child process memory
    if(prctl(PR_SET_DUMPABLE, 1)<0){
        throwScalerExceptionS(ErrCode::PTRCTL_FAIL, "PR_SET_DUMPABLE failed because: %s", strerror(errno));
    }

    //Allow tracing of this process
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        throwScalerExceptionS(ErrCode::PTRACE_FAIL, "PTRACE_TRACEME failed because: %s", strerror(errno));
    }

    /* Replace this process's image with the given program */
    if(execl(programname, programname, NULL)<0) {
        throwScalerExceptionS(ErrCode::TARGET_EXEC_FAIL, "execl target failed because: %s", strerror(errno));
    }
}

std::string executableName;

int main(int argc, char **argv) {
    pid_t childPid;

    executableName = argv[1];
    if (argc < 2) {
        ERR_LOG("Expected <program absolute path>\n");
        return -1;
    }
    childPid = fork();
    if (childPid == 0)
        run_target(argv[1]);
    else if (childPid > 0) {
        DBG_LOG("Debugger run");

        std::this_thread::sleep_for(std::chrono::seconds(1));

        install([](std::string fileName, std::string funcName) -> bool {
            if (fileName == "/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.28") {
                //todo: User should be able to specify name here. Since they can change filename
                return false;
            } else {
                return true;
            }
        }, INSTALL_TYPE::BRKPOINT_PTRACE, childPid);
    } else {
        perror("fork");
        return -1;
    }
    return 0;
}