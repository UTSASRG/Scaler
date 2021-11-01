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


    printf("Calling funcEverything\n");
    structparm strP;
    strP.a = 15;
    strP.b = 215;
    strP.d = 0.32551278;
    structparm s;
    s.a = 25;
    s.d = 325823.21121251;
    int e = 565, f = 11256, g = 121894, h = 69783, i = 245, j = 12357, k = 88776;
    long double ld = 8371652.2765257;
    double m = 2871.2746362, n = 271232.3782;
    __m256 y = _mm256_set_ps(1278611.1225, 21852.576284, 21124566.78088, 921734562.23, 0.28914970, 12.021315, 214.52160, 162.0242);
    __m512 z = _mm512_set_ps(224152.215680, 89794.021145, 89065436.213, 883.340, 10251.0122, 121234.025251, 14567.0567, 16567.0567, 2234.607, 482.03, 653.02, 879.03, 46310.07, 12342.07376, 142.021412, 16.022);
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);

    printf("Calling callFuncA\n");
    callFuncA();

    printf("Calling callFunc1000\n");
    callFunc1000();

    return 0;
}