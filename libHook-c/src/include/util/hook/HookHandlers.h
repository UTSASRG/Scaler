
#ifndef SCALER_HOOKHANDLERS_H
#define SCALER_HOOKHANDLERS_H

#include <type/ExtSymInfo.h>

extern "C" {

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

__attribute__((used)) void asmHookHandler();

#define redzoneJumperDecl(N) void __attribute__((used, naked)) redzoneJumper##N();

redzoneJumperDecl(0) ;
redzoneJumperDecl(1) ;
redzoneJumperDecl(2) ;
redzoneJumperDecl(3) ;
redzoneJumperDecl(4) ;
redzoneJumperDecl(5) ;
redzoneJumperDecl(6) ;
redzoneJumperDecl(7) ;
redzoneJumperDecl(8) ;
redzoneJumperDecl(9) ;

}
#endif