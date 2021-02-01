#include <iostream>
#include <installTest.h>
#include <FuncWithDiffParms.h>

using namespace std;

int main() {

    //调main
//    printf("Library B\n");
//
//    callFuncA();
//    callFunc1000();

    funcA();

    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
    return 0;
}

