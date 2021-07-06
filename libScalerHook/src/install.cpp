#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHookAsm.hh>

extern bool AUTO_INSTALL_ENABLED=false;
/**
 * Manual Installation
 */
void install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    if (!AUTO_INSTALL_ENABLED) {
        scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
        libPltHook->install(filterCallB);
    }
}

