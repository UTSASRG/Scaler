
#include "hook.hh"

static bool PRELOADED = false;

enum INSTALL_TYPE {
    ASM = 0,
    BRKPOINT_PTRACE = 1,
    BRKPOINT = 2
};

void install(scaler::Hook::SYMBOL_FILTER filterCallB);

void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type, pid_t childPID);

void install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type);

void uninstall(INSTALL_TYPE type);