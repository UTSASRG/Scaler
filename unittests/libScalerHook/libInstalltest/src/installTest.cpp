#include <installTest.h>
#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>

extern "C" {

void callFuncA() {
    printf("Inside callFuncA\n");
    funcA();
}

void callFunc1000() {
    printf("Inside callFunc1000\n");
    func1000();
}



}