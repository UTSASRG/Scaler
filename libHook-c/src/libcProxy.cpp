#include <util/tool/StringTool.h>
#include <util/tool/Logging.h>
#include <dlfcn.h>
#include <util/tool/FileTool.h>
#include <util/config/Config.h>
#include <exceptions/ScalerException.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <util/hook/proxy/libcProxy.h>
#include <util/hook/ExtFuncCallHook.h>
#include <util/hook/HookContext.h>
#include <util/hook/ExtFuncCallHook.h>
#include <cxxabi.h>

main_fn_t real_main;

extern "C" {
scaler::Vector<HookContext *> threadContextMap;
#ifndef MANUAL_INSTALL

int doubletake_main(int argc, char **argv, char **envp) {
    INFO_LOGS("libHook-c Ver %s", CMAKE_SCALERRUN_VERSION);
    INFO_LOGS("Main thread id is%lu", pthread_self());

    char pathName[PATH_MAX];
    if (!getcwd(pathName, sizeof(pathName))) {
        fatalErrorS("Cannot get cwd because: %s", strerror(errno));
    }


    std::stringstream ss;
    ss << pathName << "/" << "scalerdata_" << getunixtimestampms();
    scaler::ExtFuncCallHook::getInst(ss.str())->install();
    //Calculate the main application time


    HookContext *curContextPtr = curContext;
    curContextPtr->curFileId = 0;
    curContextPtr->endTImestamp = 0;
    curContextPtr->startTImestamp = getunixtimestampms();
    curContextPtr->isMainThread = true;

    /**
     * Register this thread with the main thread
     */
    threadContextMap.pushBack(curContextPtr);

    int ret = real_main(argc, argv, envp);
    curContextPtr->endTImestamp = getunixtimestampms();
    saveData(curContextPtr);
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

void exit(int __status) {
    auto realExit = (exit_origt) dlsym(RTLD_NEXT, "exit");
    curContext->endTImestamp = getunixtimestampms();
    saveData(curContext, true);
    realExit(__status);
}


#endif
}
