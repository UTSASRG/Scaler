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
    assert(bypassCHooks == SCALER_TRUE);
    //Initialize saving data structure
    curContext = new HookContext(
            scaler::ExtFuncCallHook::instance->elfImgInfoMap.getSize(),
            scaler::ExtFuncCallHook::instance->hookedExtSymSize);


    if (!curContext) {
        fatalError("Failed to allocate memory for Context");
        return false;
    }

    //RuntimeInfo newInfo;

    bypassCHooks = SCALER_FALSE;
    return true;
}

__thread HookContext *curContext __attribute((tls_model("initial-exec")));

__thread uint8_t bypassCHooks __attribute((tls_model("initial-exec"))) = SCALER_TRUE; //Anything that is not SCALER_FALSE should be treated as SCALER_FALSE


}