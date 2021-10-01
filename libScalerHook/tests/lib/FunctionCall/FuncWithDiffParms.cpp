#include <cstdio>
#include <FuncWithDiffParms.h>
#include <string>
#include <link.h>
#include <thread>

extern "C" {

void funcA() {
    printf("Inside Function A\n");
}

void loopDelay(long long times) {
    printf("loopdelay\n");
    long long sum = 0;
    for (long long i = 0; i < times; ++i) {
        ++sum;
    }
}

void sleepDelay(long long millseconds) {
    printf("Sleep delay\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(millseconds));
}

void funcB(int a) {
    printf("Inside Function B\n");
}

void funcC(int a, int b) {
    printf("Inside Function C\n");
}

void funcD(int a, int b, int c) {
    printf("Inside Function D\n");
}

int funcE(int a, int b, int c) {
    printf("Inside Function E\n");
    return 1;
}
uint64_t funcTiming() {
//    return __rdtsc();
    return 0;
}

void resolveSystemFunc() {
    system("");
    system("");
    system("");
}

void funcEverything(int e, int f, structparm s, int g, int h, long double ld,
                    double m, __m256 y, __m512 z, double n, int i, int j, int k) {

    printf("Inside Function Everything\n");

}

void *getFuncAddr(std::string funcName) {
    if (funcName == "funcA") {
        return (void *) funcA;
    } else if (funcName == "funcB") {
        return (void *) funcB;
    } else if (funcName == "funcC") {
        return (void *) funcC;
    } else if (funcName == "funcD") {
        return (void *) funcD;
    } else if (funcName == "funcE") {
        return (void *) funcE;
    } else if (funcName == "funcEverything") {
        return (void *) funcEverything;
    }
    return nullptr;
}

void *findRdbg() {
    r_debug *_myRdebug = nullptr;
    const ElfW(Dyn) *dyn = _DYNAMIC;
    for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
        if (dyn->d_tag == DT_DEBUG)
            _myRdebug = (struct r_debug *) dyn->d_un.d_ptr;
    return _myRdebug;
}

void *findDYNAMIC() {
    return _DYNAMIC;
}

int EXTVAR_VAR1 = 1;
static int EXTVAR_VAR2 = 2597;
}

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    int j;

    printf("name=%s (%d segments)\n", info->dlpi_name,
           info->dlpi_phnum);

    for (j = 0; j < info->dlpi_phnum; j++)
        printf("\t\t header %2d: address=%10p\n", j,
               (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr));
    return 0;
}


float funcF(float a, float b, float c) {
    return a + b + c;
}

int A::asdf = 1;


