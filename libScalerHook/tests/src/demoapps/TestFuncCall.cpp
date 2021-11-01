#include <cstdio>
#include <FuncWithDiffParms.h>
#include <CallFunctionCall.h>
#include <TenThousandFunc.h>
#include <util/hook/install.h>
#include <sys/prctl.h>
#include <thread>
#include <util/tool/MemTool.h>
#include <cassert>

using namespace std;

int main() {
    pthread_t pt1 = pthread_self();
    assert(pt1!=-1);
    printf("pt1=%lu\n", pt1);
    printf("pt1=%lu\n", myGetThreadID());

    printf("Calling funcA\n");
    funcA();
    funcA();

    printf("Calling funcB\n");
    funcB(1);

    printf("Calling funcC\n");
    funcC(1, 2);

    printf("Calling funcD\n");
    funcD(1, 2, 3);

    printf("Calling funcE\n");
    funcE(1, 2, 3);

    printf("Calling funcEverything\n");
    structparm strP;
    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);

    printf("Calling callFuncA\n");
    callFuncA();

    printf("Calling callFunc1000\n");
    callFunc1000();
    int a[] = {1, 2, 3, 4, 5};
    printf("a[]={1,2,3,4,5} starts at %p\n", a);

    printf("My id is: %lu\n", pthread_self());

//    prctl(PR_SET_DUMPABLE, 1);

//    while (1) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
    uninstall(INSTALL_TYPE::ASM);
    return 0;
}