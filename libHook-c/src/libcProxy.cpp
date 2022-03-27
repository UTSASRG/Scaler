#include <util/tool/StringTool.h>
#include <util/tool/Logging.h>
#include <dlfcn.h>
#include <util/tool/FileTool.h>
#include <util/config/Config.h>
#include <exceptions/ScalerException.h>
#include <util/tool/Timer.h>
#include <util/hook/proxy/libcProxy.h>
#include <util/hook/ExtFuncCallHook.h>

main_fn_t real_main;

extern "C" {

int doubletake_main(int argc, char **argv, char **envp) {
    INFO_LOGS("libHook-c Ver %s", CMAKE_SCALERRUN_VERSION);
    INFO_LOGS("Main thread id is%lu", pthread_self());
    scaler::ExtFuncCallHook::getInst()->install();
    int ret = real_main(argc, argv, envp);
    return ret;
}


int doubletake_libc_start_main(main_fn_t main_fn, int argc, char **argv, void (*init)(), void (*fini)(),
                               void (*rtld_fini)(), void *stack_end) {
    using namespace scaler;
    // Find the real __libc_start_main
    auto real_libc_start_main = (decltype(__libc_start_main) *) dlsym(RTLD_NEXT, "__libc_start_main");
    if (!real_libc_start_main) {
        fatalError("Cannot find __libc_start_main.");
        return -1;
    }
    // Save the program's real main function
    real_main = main_fn;
    // Run the real __libc_start_main, but pass in doubletake's main function
    return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}
}
