#include <cstdio>
#include <FuncWithDiffParms.h>
#include <string>
#include <link.h>

extern "C" {

void funcA() {
    int i = 1;
    int j = 2;
    int k = i + j;
    printf("This is Function A\n");
}

void funcB(int a) {
}

void funcC(int a, int b) {
}

void funcD(int a, int b, int c) {
    printf("This is Function D\n");
}

int funcE(int a, int b, int c) {
    return 1;
}


void funcEverything(int e, int f, structparm s, int g, int h, long double ld,
                    double m, __m256 y, __m512 z, double n, int i, int j, int k) {

    printf("This is Function Everything\n");
    system("");

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
int A::asdf = 1;



