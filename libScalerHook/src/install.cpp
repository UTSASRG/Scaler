#include <util/hook/install.h>
#include <util/hook/ExtFuncCallHook.hh>

void install() {
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->install();
}

