#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <utility>
#include <exceptions/ScalerException.h>

//todo: If use ptrace, asm doesn't have to be imported. Use macro to handle this?
/**
 * Manual Installation
 */

#include <sys/resource.h>

scaler::ExtFuncCallHookAsm *libPltHook = nullptr;

bool install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type, pid_t childPID) {
    if (type == INSTALL_TYPE::ASM) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        return libPltHook->install(filterCallB);
    } else {
        ERR_LOGS("Unsupported type %d", type);
        return false;
    }
}

bool install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    return install(filterCallB, INSTALL_TYPE::ASM);
}

bool uninstall(INSTALL_TYPE type) {
    if (type == INSTALL_TYPE::ASM) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        return libPltHook->uninstall();
    } else {
        ERR_LOGS("Unsupported type %d", type);
        return false;
    }
}