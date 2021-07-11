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

void run_target(const char *programname) {
    DBG_LOGS("target started. will run '%s'", programname);
    prctl(PR_SET_DUMPABLE, 1);
    /* Allow tracing of this process */
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        ERR_LOG("ptrace\n");
        return;
    }

    /* Replace this process's image with the given program */
    execl(programname, programname, NULL);
}

int main(int argc, char **argv) {
    pid_t childPid;

    if (argc < 2) {
        ERR_LOG("Expected <program absolute path>\n");
        return -1;
    }
    childPid = fork();
    if (childPid == 0)
        run_target(argv[1]);
    else if (childPid > 0) {
        DBG_LOG("Debugger run");


        std::this_thread::sleep_for (std::chrono::seconds(1));

        install([](std::string fileName, std::string funcName) -> bool {
            if (fileName ==
                "/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/tests/libScalerHook-demoapps-FuncCall") {
                DBG_LOGS("%s:%s hooked", fileName.c_str(), funcName.c_str());
                //todo: User should be able to specify name here. Since they can change filename
                return true;
            } else {
                return false;
            }
        }, INSTALL_TYPE::BRKPOINT_PTRACE, childPid);
    } else {
        perror("fork");
        return -1;
    }
    return 0;
}