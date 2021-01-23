#include <iostream>

#include <FuncWithDiffParms.h>
#include <installTest.h>
#include <util/hook/install.h>

using namespace std;

extern char __startpltsec;

int main() {
    //void *pltSecStartAddr = load();
    //printf("%p,%p\n", pltSecStartAddr, &__startpltsec);
//    printf("Library A\n");
    //load();
    install();
    funcA();
    funcB(1);
    funcC(2, 3);
    funcD(2, 3, 4);
//
//    funcA();

    //调main
//    printf("Library B\n");

    callFuncA();
    callFunc1000();
    return 0;
}

