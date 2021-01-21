#include <iostream>

#include <FuncWithDiffParms.h>
#include <util/hook/install.h>

using namespace std;


int main() {
    install();
    funcA();
    funcA();
    funcA();

    return 0;
}

