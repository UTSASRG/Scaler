


#include <cstdlib>
#include <sstream>
#include <util/tool/Logging.h>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;


void printUsage(void) {
    fprintf(stderr,
            "ScalerRun [VARIANT] <target_program> {target_program_options} ...\n"
            "VARIANT:                 COUNT/TIME/TRACE\n"
    );
}

void printVersion() {
    INFO_LOGS("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
}


int runScaler(std::string mode, int argc, char **argv) {

    std::string scalerBinPath = "/usr/local/scaler/libScalerHook-HookAuto-" + mode + ".so";
    DBG_LOGS("Scaler binary is at %s according to the configuration", scalerBinPath.c_str());
    std::stringstream ss;
    ss << "LD_PRELOAD=" << scalerBinPath << " ";
    for (int i = 0; i < argc; ++i) {
        ss << argv[i] << " ";
    }
    DBG_LOGS("%s\n", ss.str().c_str());
    return system(ss.str().c_str());
}

int main(int argc, char **argv) {
    INFO_LOGS("ScalerRun Ver %s", CMAKE_SCALERRUN_VERSION);
    //Parse Commandline arguments

    if (argc <= 2) {
        printUsage();
        exit(0);
    }

    if (strncmp(argv[1], "COUNT", 5) == 0) {
        INFO_LOG("COUNT mode");
    } else if (strncmp(argv[1], "TIME", 4) == 0) {
        INFO_LOG("TIME mode");
    } else if (strncmp(argv[1], "TRACE", 5) == 0) {
        INFO_LOG("TRACE mode");
    } else {
        ERR_LOGS("Unsupported mode: %s", argv[2]);
        printUsage();
        exit(0);
    }

    return runScaler(argv[1], argc - 2, argv + 2);
}