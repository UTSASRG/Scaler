/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>
#include <installTest.h>
#include <FuncWithDiffParms.h>
#include <cassert>
#include <util/hook/install.h>
#include <util/tool/ProcInfoParser.h>

using namespace std;

int main() {
    scaler::PmParser_Linux pmP;
    std::string curFileName = pmP.curExecAbsolutePath;
    install([](std::string fileName, std::string funcName) -> bool {
        if (funcName.find("funcA") != std::string::npos) {
            return true;
        } else {
            return false;
        }
    });

    printf("Calling funcA\n");
    funcA();

//    structparm s;
//    int e, f, g, h, i, j, k;
//    long double ld;
//    double m, n;
//    __m256 y;
//    __m512 z;
//    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
//
//    float floatRlt=funcF(1.0, 2.0, 3.0);
//    printf("%f\n",floatRlt);
//
//    callFuncA();
    return 0;
}

