#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <utility>
#include <util/hook/ExtFuncCallHookPtrace.h>
#include <exceptions/ScalerException.h>

//todo: If use ptrace, asm doesn't have to be imported. Use macro to handle this?
/**
 * Manual Installation
 */




void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type) {
    if (type == INSTALL_TYPE::ASM) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        libPltHook->install(filterCallB);
    } else if (type == INSTALL_TYPE::BRKPOINT_PTRACE) {
        throwScalerException("Wrong interface, you have to pass executable full path for BRKPOINT_PTRACE")
    } else if (type == INSTALL_TYPE::BRKPOINT) {
        throwScalerException("Breakpoint not implemented yet. Can't install.")
    }
}


void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type, pid_t childPID) {
    if (type == INSTALL_TYPE::ASM) {
        throwScalerException("Wrong interface, you don't need to specify childPID")
    } else if (type == INSTALL_TYPE::BRKPOINT_PTRACE) {
        scaler::ExtFuncCallHookPtrace *libPltHook = scaler::ExtFuncCallHookPtrace::getInst(childPID);
        libPltHook->install(filterCallB);
    } else if (type == INSTALL_TYPE::BRKPOINT) {
        throwScalerException("Breakpoint not implemented yet. Can't install.")
    }
}

void install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    install(filterCallB, INSTALL_TYPE::ASM);
}
