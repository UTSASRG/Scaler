/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <FuncWithDiffParms.h>
#include <dlfcn.h>

typedef void (*FuncAType)();

//This example will not work
int main() {
    printf("First dlopen start. Scaler should install on all APIs from funcCall.so and dependencies.\n");
    void *libHandle = dlopen("/home/steven/Projects/Scaler-eurosys2024/cmake-build-debug/libHook-c/tests/libTestlib-FuncCall.so",
                             RTLD_LAZY);
    printf("LibHandle=%p\n",libHandle);
    printf("Second dlopen start. Scaler should not install anything because it is a new library\n");
    libHandle = dlopen("/home/steven/Projects/Scaler-eurosys2024/cmake-build-debug/libHook-c/tests/libTestlib-FuncCall.so",
                       RTLD_LAZY);

    if (!libHandle) {
        fprintf(stderr, "Cannot open libTestlib-FuncCall.so\n");
    } else {
        printf("First dlsym start. Scaler will install this new symbol\n");
        auto funcAPtr = (FuncAType) dlsym(libHandle, "funcA");
        printf("Second dlsym start. Scaler will install not install existing symbol\n");
        auto funcAPtr1 = (FuncAType) dlsym(libHandle, "funcA");
        assert(funcAPtr==funcAPtr1);
        if (!funcAPtr) {
            fprintf(stderr, "Cannot find funcA\n");
        } else {
            funcAPtr();
        }
    }

    return 0;
}