#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHook.hh>

void install() {

}

typedef int (*main_fn_t)(int, char**, char**);
main_fn_t real_main;

int doubletake_main(int argc, char** argv, char** envp) {
    //Initialization
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->install();

    // Call the program's main function
    int ret = real_main(argc, argv, envp);

    return ret;
}

extern "C" int __libc_start_main(main_fn_t, int, char**, void (*)(), void (*)(), void (*)(), void*) __attribute__((weak, alias("doubletake_libc_start_main")));

extern "C" int doubletake_libc_start_main(main_fn_t main_fn, int argc, char** argv, void (*init)(), void (*fini)(), void (*rtld_fini)(), void* stack_end) {
    // Find the real __libc_start_main
    auto real_libc_start_main = (decltype(__libc_start_main)*)dlsym(RTLD_NEXT, "__libc_start_main");
    // Save the program's real main function
    real_main = main_fn;
    // Run the real __libc_start_main, but pass in doubletake's main function
    return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}
