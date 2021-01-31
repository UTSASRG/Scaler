#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHook.hh>

/**
 * Manual Installation
 */
void install(scaler::Hook::SYMBOL_FILTER filterCallB) {
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->install(filterCallB);
}

