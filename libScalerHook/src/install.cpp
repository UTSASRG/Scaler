#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <utility>
#include <util/hook/ExtFuncCallHookPtrace.h>
#include <util/hook/ExtFuncCallHookBrkpoint.h>
#include <exceptions/ScalerException.h>

//todo: If use ptrace, asm doesn't have to be imported. Use macro to handle this?
/**
 * Manual Installation
 */

#include <sys/resource.h>
scaler::ExtFuncCallHookAsm *libPltHook = nullptr;

void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type) {
    const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64 Mb
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }


    if (type == INSTALL_TYPE::ASM) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        libPltHook->install(filterCallB);
    } else if (type == INSTALL_TYPE::BRKPOINT_PTRACE) {
        throwScalerException(ErrCode::WRONG_INTERFACE,
                             "Wrong interface, you have to pass executable full path for BRKPOINT_PTRACE");
    } else if (type == INSTALL_TYPE::BRKPOINT) {
        scaler::ExtFuncCallHookBrkpoint *libPltHook = scaler::ExtFuncCallHookBrkpoint::getInst();
        libPltHook->install(filterCallB);
    }
}


void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type, pid_t childPID) {
    if (type == INSTALL_TYPE::ASM) {
        throwScalerException(ErrCode::WRONG_INTERFACE, "Wrong interface, you don't need to specify childPID");
    } else if (type == INSTALL_TYPE::BRKPOINT_PTRACE) {
        scaler::ExtFuncCallHookPtrace *libPltHook = scaler::ExtFuncCallHookPtrace::getInst(childPID);
        libPltHook->install(filterCallB);
    } else if (type == INSTALL_TYPE::BRKPOINT) {
        throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "Wrong interface, you don't need to specify childPID.");
    }
}

void install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    install(filterCallB, INSTALL_TYPE::ASM);
}

void uninstall(INSTALL_TYPE type) {
    if (type == INSTALL_TYPE::ASM) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        libPltHook->uninstall();
    }
}