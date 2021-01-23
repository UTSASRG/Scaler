#include <util/hook/ExtFuncCallHook.hh>


//__attribute__((constructor)) void libConstructor() {
//    printf("Constructor\n");
//    scaler::Hook *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
//    libPltHook->install();
//}

//__attribute__((destructor)) void libDeConstructor() {
//    printf("DeConstructor\n");
//}

void* load() {
    printf("Constructor\n");
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->locateRequiredSecAndSeg();
    auto &curELFImgMap = libPltHook->elfImgInfoMap[0];
    return curELFImgMap.pltSecStartAddr;
}
void *install() {
    printf("Constructor\n");
    scaler::ExtFuncCallHook_Linux *libPltHook = scaler::ExtFuncCallHook_Linux::getInst();
    libPltHook->locateRequiredSecAndSeg();
//    libPltHook->pmParser.printPM();
    libPltHook->install();
    auto &curELFImgMap = libPltHook->elfImgInfoMap[0];
    return curELFImgMap.pltSecStartAddr;
}