#include <iostream>
#include <installTest.h>
#include <FuncWithDiffParms.h>
#include <cassert>
#include <util/hook/install.h>
#include <util/tool/ProcInfoParser.h>

using namespace std;

int main() {
    scaler::PmParser_Linux pmP;
    std::string curFileName = pmP.curExecFileName;
    install([](std::string fileName, std::string funcName) -> bool {
        if (fileName.find("TestInstall") != std::string::npos) {
            return true;
        } else {
            return false;
        }
    });

    funcA();

    structparm s;
    int e, f, g, h, i, j, k;
    long double ld;
    double m, n;
    __m256 y;
    __m512 z;
    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);

    float floatRlt=funcF(1.0, 2.0, 3.0);
    printf("%f\n",floatRlt);

    callFuncA();
    return 0;
}

