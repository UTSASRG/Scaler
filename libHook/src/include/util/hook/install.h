
#include "hook.hh"

static bool PRELOADED = false;

enum INSTALL_TYPE {
    ASM = 0,
    BRKPOINT_PTRACE = 1,
    BRKPOINT = 2
};

bool install(scaler::Hook::SYMBOL_FILTER filterCallB);

bool install(scaler::Hook::SYMBOL_FILTER filterCallB, INSTALL_TYPE type, pid_t childPID=0);

bool uninstall(INSTALL_TYPE type);