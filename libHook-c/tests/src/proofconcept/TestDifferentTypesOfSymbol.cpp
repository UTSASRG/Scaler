#include <cstdio>
#include "TestDifferentTypesOfSymbolLib.h"

void functionInternal(){
    printf("Function Internal\n");
}
extern int a;


int main(){
    functionInternal();
    funcExternalWeak();
    functionExternal();
}