#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHook.hh>

extern bool AUTO_INSTALL_ENABLED=false;
/**
 * Manual Installation
 */
void install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    if (!AUTO_INSTALL_ENABLED) {
        scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
        libPltHook->install(filterCallB);
    }
}

