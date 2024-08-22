/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <cstdio>
#include <cstdlib>
#include <FuncWithDiffParms.h>
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
