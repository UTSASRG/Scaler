#include <util/hook/ExtFuncCallHook.hh>


//__attribute__((constructor)) void libConstructor() {
//    printf("Constructor\n");
//    scaler::Hook *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
//    libPltHook->install();
//}

//__attribute__((destructor)) void libDeConstructor() {
//    printf("DeConstructor\n");
//}

void install(){
    printf("Constructor\n");
    scaler::Hook *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->install();
}