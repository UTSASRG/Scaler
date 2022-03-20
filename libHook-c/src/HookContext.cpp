#include <util/hook/HookContext.h>

extern "C" {
HookContext::HookContext(ssize_t libFileSize, ssize_t hookedSymbolSize) : memArrayHeap(8192) {
//    //bool isMainThread = scaler::Config::mainthreadID == pthread_self(); //Dont double initialize
//
//    //Initialize root node
//    rootNode = memArrayHeap.allocArr(1);
//    //Initialize root node child size. id=0 is always the main application
//    rootNode->childrenSize = scaler_extFuncCallHookAsm_thiz->elfImgInfoMap[0].hookedSymbolSize;
//    //Initialize children
//    rootNode->children = memArrayHeap.allocArr(rootNode->childrenSize);
//    rootNode->scalerId = -1; //Indiacate root node
//
//    initialized = SCALER_TRUE;
}

HookContext::~HookContext() {

}

}