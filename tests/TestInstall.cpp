#include <iostream>

#include <FuncWithDiffParms.h>
#include <installTest.h>
#include <util/hook/install.h>

using namespace std;


int main() {
    install();
//    printf("Library A\n");
//    funcA();
//    funcA();
//    funcA();
//    funcA();
    funcB(1);
    funcC(2, 3);
    funcD(2, 3, 4);
//
//    funcA();

    //调main
    printf("Library B\n");

    callFuncA();
    callFunc1000();
    return 0;
}

