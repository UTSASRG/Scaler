#include <iostream>
#include <thread>
#include <util/hook/install.h>
#include <FuncWithDiffParms.h>
#include <installTest.h>
#include <util/tool/ProcInfoParser.h>

using namespace std;


int main() {

    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename
        if (fileName == "/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/libscalerhook.so") {
            return true;
        }
        else {
            return true;
        }
    });


//    scaler::PmParser_Linux parser;
//    parser.printPM();
//
//    printf("funcA\n");
//    funcA();
//    malloc(190);
//    callFuncA();
//    structparm strP;
//
//    structparm s;
//    int e, f, g, h, i, j, k;
//    long double ld;
//    double m, n;
//    __m256 y;
//    __m512 z;
//    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
//    printf("Hello\n");
//    printf("Hello %d %f\n", 1, 1.23);
//
    std::cout << "Hello world" << 1 << 1.5 << endl;

    return 0;
}

