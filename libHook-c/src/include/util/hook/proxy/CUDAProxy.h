
#ifndef SCALER_CUDAPROXY_H
#define SCALER_CUDAPROXY_H

#ifdef CUDA_ENABLED
#include <cuda.h>

CUresult CUDAAPI cuGetProcAddress_proxy(const char *symbol, void **pfn, int cudaVersion, cuuint64_t flags, CUdriverProcAddressQueryResult *symbolStatus);
#endif

#endif //SCALER_CUDAPROXY_H
