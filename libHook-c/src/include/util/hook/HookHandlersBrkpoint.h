
#ifndef SCALER_HOOKHANDLERS_H
#define SCALER_HOOKHANDLERS_H

#include <type/ExtSymInfo.h>
#include <type/Breakpoint.h>

extern "C" {

void brkpointEmitted(int signum, siginfo_t *siginfo, void *context);

void skipBrkPoint(scaler::Breakpoint &bp, ucontext_t *context);
}
#endif