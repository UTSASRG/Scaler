#include <installTest.h>
#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>

extern "C" {

void callFuncA() {
    printf("callFuncA\n");
    funcA();
}

void callFunc1000() {
    printf("callFunc1000\n");

    func1000();
}

}