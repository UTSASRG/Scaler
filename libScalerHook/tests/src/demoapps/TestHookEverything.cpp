#include <cstdio>
#include <FuncWithDiffParms.h>
#include <CallFunctionCall.h>
#include <TenThousandFunc.h>
#include <util/hook/install.h>
#include <util/tool/StringTool.h>

using namespace std;

int main() {
    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename

        if (scaler::strEndsWith(fileName, "ld-2.31.so")) {
            return false;
        } else if (scaler::strEndsWith(fileName, "libScalerHook-HookManual.so")) {
            return false;
        }else if(scaler::strEndsWith(fileName, "libstdc++.so.6.0.28")){
            return false;
        }
        else if(scaler::strEndsWith(fileName, "libdl-2.31.so")){
            return false;
        }
        else {
            fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
            return true;
        }

    });


    printf("Calling funcA\n");
    funcA();

    printf("Calling funcB\n");
    funcB(1);

    printf("Calling funcC\n");
    funcC(1, 2);

    printf("Calling funcD\n");
    funcD(1, 2, 3);

    printf("Calling funcE\n");
    funcE(1, 2, 3);

    printf("Calling func1472\n");
    func1472();


    printf("Calling funcEverything\n");
    structparm strP;
    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);

    printf("Calling callFuncA\n");
    callFuncA();

    printf("Calling callFunc1000\n");
    callFunc1000();

    return 0;
}