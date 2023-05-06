#include <cstdio>
#include <string>

void  funcInternal1(std::string str); //__attribute__((__visibility__("hidden")))

void __attribute__((weak)) funcExternalWeak();

void functionExternal();
