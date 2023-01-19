#include <util/hook/LogicalClock.h>

uint32_t threadNum = 0;
uint64_t threadAttributionClock = 0;
uint64_t wallclockSnapshot = 0;
