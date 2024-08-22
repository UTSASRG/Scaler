#include <util/hook/proxy/CUDAProxy.h>
#include <util/tool/Logging.h>
#include "util/hook/ExtFuncCallHook.h"
#ifdef CUDA_ENABLED

CUresult CUDAAPI cuGetProcAddress_proxy(const char *symbol, void **pfn, int cudaVersion, cuuint64_t flags,
                                        CUdriverProcAddressQueryResult *symbolStatus) {
    //todo: Consider multithreading
    INFO_LOGS("cuGetProcAddress Interception Start for %s", symbol);
    CUresult cudaResult = cuGetProcAddress(symbol, pfn, cudaVersion, flags, symbolStatus);
    if(cudaResult != CUDA_SUCCESS || *pfn==nullptr){
        ERR_LOGS("Scaler cannot hook CUDA function:%s because cuGetProcAddress failed",symbol);
        return cudaResult;
    }

    scaler::ExtFuncCallHook *instance = scaler::ExtFuncCallHook::getInst();
    void* scalerReturnAddr=nullptr;
    instance->installDlSym(__name,*pfn,scalerReturnAddr);
    *pfn = scalerReturnAddr;

    return cudaResult;
}

#endif