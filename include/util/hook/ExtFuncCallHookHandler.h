#ifndef SCALER_EXTFUNCCALLHOOKHANDLER_H
#define SCALER_EXTFUNCCALLHOOKHANDLER_H

#include <map>
#include <vector>


#define DECLAR_C_HOOK_HANDLER(suffix)

//void *cHookHanlderSec(int index, void *funcAddr) {

//    funcAddr = reinterpret_cast<void *>(uint64_t(funcAddr) - 0xD);
//    int funcId = ((uint64_t) funcAddr - (uint64_t) ptrPltSec) / 16;
//
//    if (!realAddrResolved[funcId]) {
//        void *curGotFuncAddr = plthook_GetPltAddress(&myPltHook, funcId);
//        if (hookedAddrs[funcId] != curGotFuncAddr) {
////            printf("Address is now resolved，replace hookedAddrs with correct value\n");
//            //Update address and set it as correct address
//            realAddrResolved[funcId] = true;
//            hookedAddrs[funcId] = curGotFuncAddr;
//            return hookedAddrs[funcId];
//        } else {
////            printf("%s is not initialized, execute orignal PLT code\n", hookedNames[funcId]);
//            SCALER_HOOK_IN_HOOK_HANDLER = false;
//            //Execute original PLT function
//            return oriPltOpCodesArr + funcId * 18;
//        }
//    }
//
//    if (SCALER_HOOK_IN_HOOK_HANDLER) {
//        return hookedAddrs[funcId];
//    }
//
//    SCALER_HOOK_IN_HOOK_HANDLER = true;
//    malloc(1);
//    malloc(2);
//    malloc(3);
////    printf("Here I am!!! %d", 1);
//    SCALER_HOOK_IN_HOOK_HANDLER = false;
//    return hookedAddrs[funcId];
//}

//todo: 0xD and 16 is platform specific
#define DECLAR_C_HOOK_HANDLER(suffix) \
void *cHookHanlder##suffix(int index, void *funcAddr) { \
    funcAddr = reinterpret_cast<void *>(uint64_t(funcAddr) - 0xD); \
    int funcId = ((uint64_t) funcAddr - (uint64_t) ptrPlt##suffix) / 16; \
    if (!realAddrResolved[funcId]) {\
        void *curGotFuncAddr = plthook_GetPltAddress(&myPltHook, funcId);\
        if (hookedAddrs[funcId] != curGotFuncAddr) {\
            realAddrResolved[funcId] = true;\
            hookedAddrs[funcId] = curGotFuncAddr;\
            return hookedAddrs[funcId];\
        } else {\
            SCALER_HOOK_IN_HOOK_HANDLER = false;\
            //Execute original PLT function\
            return oriPltOpCodesArr + funcId * 18;\
        }\
    }\
    if (SCALER_HOOK_IN_HOOK_HANDLER) {\
        return hookedAddrs[funcId];\
    }\
    SCALER_HOOK_IN_HOOK_HANDLER = true;\
    malloc(1);\
    malloc(2);\
    malloc(3);\
    SCALER_HOOK_IN_HOOK_HANDLER = false;\
    return hookedAddrs[funcId];\
}\

#endif //SCALER_EXTFUNCCALLHOOKHANDLER_H
