#include<util/hook/proxy/SystemProxy.h>
#include <unistd.h>


__pid_t fork_proxy(void) {

    __pid_t result = fork();
    if (result == 0) {
        //Child process
        //Clear recording buffer
    }
    return result;
}
