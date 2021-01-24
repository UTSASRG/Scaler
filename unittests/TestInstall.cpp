#include <iostream>

#include <installTest.h>

using namespace std;

extern char __startpltsec;

int main() {

    //调main
    printf("Library B\n");

    callFuncA();
    callFunc1000();
    return 0;
}

