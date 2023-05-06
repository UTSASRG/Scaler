#include <cstdio>
#include <string>
#include "TestDifferentTypesOfSymbolLib.h"

void funcInternal1(std::string str){
    printf("%s\n",str.c_str());
}

void __attribute__((weak)) funcExternalWeak(){
    funcInternal1("funcExternalWeak\n");
}
void functionExternal(){
    funcInternal1("functionExternal\n");
}


