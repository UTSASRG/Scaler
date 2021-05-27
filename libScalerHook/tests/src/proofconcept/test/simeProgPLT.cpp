
#include <cstdio>
#include <milionFuncLibrary.h>
#include <libtest.h>

int main() {
    funcA();
    funcB(1);
    return 0;
}

__attribute__((constructor)) void libConstructor() {
    printf("Constructor\n");
}

__attribute__((destructor)) void libDeConstructor() {
    printf("DeConstructor\n");
}