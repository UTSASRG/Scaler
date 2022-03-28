#include <cstdio>
#include <cstdlib>
#include "libtest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

int printFile(const char *fileName) {
    FILE *fptr;
    char c;
    // Open file
    fptr = fopen(fileName, "r");
    if (fptr == NULL) {
        printf("Cannot open file \n");
        exit(0);
    }
    // Read contents from file
    c = fgetc(fptr);
    while (c != EOF) {
        printf("%c", c);
        c = fgetc(fptr);
    }
    fclose(fptr);
    return 0;
}

void selfFuncA() {
    printf("Hello This is selfFuncA\n");
    int cs;
    asm volatile("mov %%cs,%0" : "=rm" (cs));
    printf("cs is 0x%2x\n",cs);
}


int main() {
    selfFuncA();
    funcA();
    printf("selfFuncA:%p\n", selfFuncA);
    printf("funcA:%p\n", funcA);
    printFile("/proc/self/maps");
    return 0;
}
