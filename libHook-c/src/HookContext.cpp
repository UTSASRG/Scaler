#include <util/hook/HookContext.h>

extern "C" {
HookContext::HookContext(ssize_t libFileSize, ssize_t hookedSymbolSize) : memArrayHeap(1) {

//    //Initialize root node
//    rootNode = memArrayHeap.allocArr(1);
//    //Initialize root node child size. id=0 is always the main application
//    rootNode->childrenSize = scaler_extFuncCallHookAsm_thiz->elfImgInfoMap[0].hookedSymbolSize;
//    //Initialize children
//    rootNode->children = memArrayHeap.allocArr(rootNode->childrenSize);
//    rootNode->scalerId = -1; //Indiacate root node



    initialized = SCALER_TRUE;
}

HookContext::~HookContext() {

}

bool initTLS() {
    short i = 1;
    //Put a dummy variable to avoid null checking
    //Initialize saving data structure
    curContext = new HookContext(
            scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
            scaler::ExtFuncCallHook::instance->hookedExtSymSize);
    curContext->callerAddr.push((void *) 0x0);
    curContext->timeStamp.push(0);
    curContext->symId.push(0);

    i += curContext->initialized;

    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }

    //RuntimeInfo newInfo;

    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE


}