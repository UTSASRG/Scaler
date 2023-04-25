#include <util/tool/StringTool.h>
#include <util/tool/Logging.h>
#include <dlfcn.h>
#include <util/tool/FileTool.h>
#include <exceptions/ScalerException.h>
#include <util/tool/Timer.h>
#include <util/tool/FileTool.h>
#include <util/hook/proxy/LibcProxy.h>
#include <util/hook/ExtFuncCallHook.h>
#include <util/hook/HookContext.h>
#include <util/hook/ExtFuncCallHook.h>
#include <cxxabi.h>

main_fn_t real_main;


bool installed = false;

extern "C" {
scaler::Vector<HookContext *> threadContextMap;
#ifndef MANUAL_INSTALL


int doubletake_main(int argc, char **argv, char **envp) {

    if (strncmp(argv[0], "time", 4) == 0 || scaler::strEndsWith(argv[0], "/time")) {
        INFO_LOGS("libHook-c Ver %s", CMAKE_SCALERRUN_VERSION);
        INFO_LOGS("Bypass hooking %s, because it is the time program.", argv[0]);
        return real_main(argc, argv, envp);
    }

    INFO_LOGS("libHook-c Ver %s", CMAKE_SCALERRUN_VERSION);
    INFO_LOGS("Main thread id is%lu", pthread_self());
    INFO_LOGS("Program Name: %s", argv[0]);

    std::stringstream ss;
    char *pathFromEnv = getenv("SCALER_OUTPUT_PATH");
    if (pathFromEnv == NULL) {
        ss << "/tmp";
    } else {
        ss << pathFromEnv;
    }

    ss << "/" << "scalerdata_" << getunixtimestampms();
    INFO_LOGS("Folder name is %s", ss.str().c_str());

    scaler::ExtFuncCallHook::getInst(ss.str())->install();
    //Calculate the main application time
    installed = true;


    HookContext *curContextPtr = curContext;
    curContextPtr->threadCreatorFileId = 0;
    curContextPtr->isMainThread = true;


    /**
     * Register this thread with the main thread
     */
    threadContextMap.pushBack(curContextPtr);

    int ret = real_main(argc, argv, envp);
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

    if (!installed) {
        realExit(__status);
    }

    saveData(curContext, true);
    realExit(__status);
}


#endif
}
