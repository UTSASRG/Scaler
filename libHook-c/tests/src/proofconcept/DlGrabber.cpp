#include <dlfcn.h>
#include <cstdio>
#include "DlGrabber.h"

using namespace std;


main_fn_t real_main;

extern "C" {

int doubletake_main(int argc, char **argv, char **envp) {

    fprintf(stderr,"Doubletake Main\n");
    return real_main(argc, argv, envp);

}

int doubletake_libc_start_main(main_fn_t main_fn, int argc, char **argv, void (*init)(), void (*fini)(),
                               void (*rtld_fini)(), void *stack_end) {
    // Find the real __libc_start_main
    auto real_libc_start_main = (decltype(__libc_start_main) *) dlsym(RTLD_NEXT, "__libc_start_main");
    if (!real_libc_start_main) {
        fprintf(stderr,"Cannot find __libc_start_main.");
        return -1;
    }
    // Save the program's real main function
    real_main = main_fn;
    // Run the real __libc_start_main, but pass in doubletake's main function

    return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}

}