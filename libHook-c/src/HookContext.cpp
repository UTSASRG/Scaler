#include <util/hook/HookContext.h>

extern "C" {
HookContext::HookContext(ssize_t libFileSize, ssize_t hookedSymbolSize) : memArrayHeap(1), timingArr(hookedSymbolSize),indexPosi(0) {

//    //Initialize root node
//    rootNode = memArrayHeap.allocArr(1);
//    //Initialize root node child size. id=0 is always the main application
//    rootNode->childrenSize = scaler_extFuncCallHookAsm_thiz->elfImgInfoMap[0].hookedSymbolSize;
//    //Initialize children
//    rootNode->children = memArrayHeap.allocArr(rootNode->childrenSize);
//    rootNode->scalerId = -1; //Indiacate root node
    timingArr.allocate(hookedSymbolSize);
    initialized = SCALER_TRUE;

    //Push a dummy value in the stack (Especially for callAddr, because we need to avoid null problem)
    callerAddr[indexPosi] = 0;
    timeStamp[indexPosi] = 0;
    symId[indexPosi] = 0;
    indexPosi = 1;

}

HookContext::~HookContext() {

}

bool initTLS() {
    //Put a dummy variable to avoid null checking
    //Initialize saving data structure
    curContext = new HookContext(
            scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
            scaler::ExtFuncCallHook::instance->allExtSymbol.getSize() + 1);


    if (!curContext) { fatalError("Failed to allocate memory for Context");
        return false;
    }

    //RuntimeInfo newInfo;

    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_FALSE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE


}