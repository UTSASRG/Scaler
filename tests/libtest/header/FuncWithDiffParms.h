#ifndef LISTTEST_H
#define LISTTEST_H

#include "immintrin.h"
#include <string>

extern "C" {

void funcA();

void funcB(int a);

void funcC(int a, int b);

void funcD(int a, int b, int c);

int funcE(int a, int b, int c);


int funcF(int a, int b, int c);

typedef struct {
    int a, b;
    double d;
} structparm;

void funcEverything(int e, int f,
                    structparm s, int g, int h,
                    long double ld, double m,
                    __m256 y,
                    __m512 z,
                    double n, int i, int j, int k);

void *getFuncAddr(std::string funcName);

void *findRdbg();

void *findDYNAMIC();

class A {
public:
    static int asdf;
};


}
#endif
