#include <iostream>
#include <thread>
#include <util/hook/install.h>
#include <FuncWithDiffParms.h>
#include "installTest.h"
#include <immintrin.h>

using namespace std;

inline unsigned long long getTimeByTSC() {
#if defined(__GNUC__)
#   if defined(__i386__)
    uint64_t x;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
        return x;
#   elif defined(__x86_64__)
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t) lo) | ((uint64_t) hi << 32);
#   else
#       error Unsupported architecture.
#   endif
#elif defined(_MSC_VER)
    __asm {
            return __rdtsc();
        }
#else
#   error Other compilers not supported...
#endif
}


#include <x86intrin.h>

//int main() {
//    install([](std::string fileName, std::string funcName) -> bool {
//        if (funcName == "funcTiming") {
//            return true;
//        } else {
//            return false;
//        }
//    });
//
//    FILE *pFile = fopen("timing.csv", "w");
//
//    for (int i = 0; i < 10000; ++i) {
//
//        auto startCycle = __rdtsc();
//
//        funcTiming();
//
//        auto endCycle = __rdtsc();
//
//        auto timePass=endCycle-startCycle;
//
//        printf("%d,", timePass);
//    }
//
//    return 0;
//}

//int main() {
//    install([](std::string fileName, std::string funcName) -> bool {
//        if (funcName == "funcTiming") {
//            return true;
//        } else {
//            return false;
//        }
//    });
//
//    FILE *pFile = fopen("timing.csv", "w");
//
//    for (int i = 0; i < 10000; ++i) {
//
//        auto startCycle = __rdtsc();
//
//        funcTiming();
//
//        auto endCycle = __rdtsc();
//
//        auto timePass=endCycle-startCycle;
//
//        printf("%d,\n", timePass);
//        fprintf(pFile,"%d,", timePass);
//        cout<<timePass<<endl;
//    }
//
//    return 0;
//}


int main() {
//    install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//
//        if (scaler::strContains(fileName, "/ld-")) {
//            return false;
//        } else if (scaler::strContains(fileName, "/libScalerHook-HookManualAsm")) {
//            return false;
//        } else if (scaler::strContains(fileName, "/libstdc++")) {
//            return false;
//        } else if (scaler::strContains(fileName, "/libdl-")) {
//            return false;
//        } else {
//            fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
//            return true;
//        }
//
//    });

    FILE *pFile = fopen("timing.csv", "w");

    for (int i = 0; i < 10000; ++i) {

        auto startCycle = __rdtsc();

        funcTiming();

        auto endCycle = __rdtsc();

        auto timePass=endCycle-startCycle;

        printf("%d,\n", timePass);
        fprintf(pFile,"%d,", timePass);
        cout<<timePass<<endl;
    }

    return 0;
}
