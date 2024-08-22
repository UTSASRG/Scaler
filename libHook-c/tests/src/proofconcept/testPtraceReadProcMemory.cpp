/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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