
#ifndef SCALER_HOOKHANDLERS_H
#define SCALER_HOOKHANDLERS_H

#include <type/ExtSymInfo.h>

extern "C" {


extern uint8_t *callIdSavers;
void __attribute__((naked)) asmTimingHandler();

/**
* A handler written in C. It calls custom handler and calculates actual function address
* In the new code, .plt and .plt.sec uses the same handler. Since we currently don't calculate
* based on the first address.
* @param callerFuncAddr The next caller
* @param oriRBPLoc The rsp location before saving all registers
* @return Original function pointer
*/
__attribute__((used)) void *preHookHandler(uint64_t nextCallAddr,ssize_t loadingId, int64_t fileId);


__attribute__((used)) void *afterHookHandler();

}
#endif