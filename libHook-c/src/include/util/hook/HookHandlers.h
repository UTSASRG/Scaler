
#ifndef SCALER_HOOKHANDLERS_H
#define SCALER_HOOKHANDLERS_H

#include <type/ExtSymInfo.h>

extern "C" {

static ssize_t bucketSize;
static uint8_t *hookBaseAddr;

void __attribute__((naked)) asmHookHandler();
/**
* A handler written in C. It calls custom handler and calculates actual function address
* In the new code, .plt and .plt.sec uses the same handler. Since we currently don't calculate
* based on the first address.
* @param callerFuncAddr The next caller
* @param oriRBPLoc The rsp location before saving all registers
* @return Original function pointer
*/
__attribute__((used)) void *preHookHandler(uint64_t nextCallAddr, uint64_t fileId);


__attribute__((used)) void *afterHookHandler();

void __attribute__((used, naked, noinline)) myPltEntry();

static uint64_t pltEntryAddr = reinterpret_cast<uint64_t>(&myPltEntry);
}
#endif