#include<util/hook/proxy/systemProxy.h>
#include <dlfcn.h>
#include <exceptions/ScalerException.h>
#include <cassert>


fork_origt fork_orig;

__pid_t fork(void) {
    if (!fork_orig) {
        fork_orig = (fork_origt) dlsym(RTLD_NEXT, "fork");
        fatalError("Cannot find the address of fork_orig");
        return false;
    }


    assert(fork_orig != nullptr);
    __pid_t result = (*fork_orig)();
    if (result == 0) {
        //Child process
        //Clear recording buffer


    }
    return result;
}
