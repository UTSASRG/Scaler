#include <cstdio>
#include <FuncWithDiffParms.h>
#include <CallFunctionCall.h>
#include <TenThousandFunc.h>

using namespace std;

int main() {

    printf("Calling funcA\n");
    funcA();

    printf("Calling funcB\n");
    funcB(1);

    printf("Calling funcC\n");
    funcC(1, 2);

    printf("Calling funcD\n");
    funcD(1, 2, 3);

    printf("Calling funcE\n");
    funcE(1, 2, 3);

    printf("Calling func1472\n");
    func1472();

#ifdef __AVX__
    printf("Calling funcEverything\n");
    structparm strP;
    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
#endif

    printf("Calling callFuncA\n");
    callFuncA();

    printf("Calling callFunc1000\n");
    callFunc1000();

    return 0;
}