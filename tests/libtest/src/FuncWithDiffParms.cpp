#include <cstdio>
#include <FuncWithDiffParms.h>

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

}
int EXTVAR_VAR1 = 1;
static int EXTVAR_VAR2 = 2597;
}
int A::asdf = 1;



