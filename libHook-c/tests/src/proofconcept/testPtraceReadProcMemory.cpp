#include <sys/ptrace.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <wait.h>

int wait4stop(pid_t pid) {
    int status = 99;
    do {
        if (waitpid(pid, &status, 0) == -1 || WIFEXITED(status) || WIFSIGNALED(status))
            return 0;
    } while(!WIFSTOPPED(status));
    return 1;
}

int get_value(int *address, pid_t pid) {
    ptrace(PTRACE_ATTACH, pid, nullptr, nullptr);
    if (!wait4stop(pid))
        perror("wait SIGSTOP of ptrace failed");
    long ret = ptrace(PTRACE_PEEKTEXT, pid, address, nullptr);
    if (errno != 0) {
        perror("ptrace error!---");
        ret = -1;
    }
    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
    return static_cast<int>(ret);
}

int main() {
    pid_t pid = 1202850;
    int *address1 = reinterpret_cast<int *>(0x55d4f33dc000);
    long data = ptrace(PTRACE_PEEKTEXT, pid, (void*)address1, 0);
    perror("error reason");
    int *address2 = reinterpret_cast<int *>(0x601070);
    printf("%ld\n", data);
//    printf("%d\n", get_value(address2, pid));
    return 0;
}