#include <cstdio>

extern "C" {

void funcA() {
    int i=1;
    int j=2;
    int k=i+j;
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

}