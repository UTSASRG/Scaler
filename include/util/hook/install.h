

//__attribute__((constructor)) void libConstructor() {
//    printf("Constructor\n");
//    scaler::Hook *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
//    libPltHook->install();
//}

//__attribute__((destructor)) void libDeConstructor() {
//    printf("DeConstructor\n");
//}

//void *load() {
//    scaler::ExtFuncCallHook_Linux::cPreHookHanlderLinuxSec(nullptr);
//    scaler::ExtFuncCallHook_Linux::cPreHookHanlderLinux(nullptr);
//    scaler::ExtFuncCallHook_Linux::cAfterHookHanlderLinux();
//    scaler::ExtFuncCallHook_Linux::(nullptr);

//}

void install();