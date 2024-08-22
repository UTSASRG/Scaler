/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <pybind11/pybind11.h>
#include <cstdio>
#include "FuncWithDiffParms.h"
#include "CallFunctionCall.h"
#include <dlfcn.h>
namespace scaler {
    typedef void (*FuncAType)();

    void callNativeFuncA(){
        funcA();
    }

    void callNativeCallFuncA(){
        callFuncA();
    }

    void callFuncAThroughDLOpen(){
        void *libHandle = dlopen( CMAKE_BINARY_DIR "/libTestlib-FuncCall.so",
                                 RTLD_LAZY);
        if (!libHandle) {
            fprintf(stderr,"Cannot open libTestlib-FuncCall.so\n");
        }else{
            auto funcAPtr = (FuncAType) dlsym(libHandle, "funcA");
            if(!funcAPtr){
                fprintf(stderr,"Cannot find funcA\n");
            }else{
                funcAPtr();
            }
        }
    }


}  // namespace hello


PYBIND11_MODULE(_TestNativeLibraryCall, m) {
m.doc() = "";
m.def("callNativeFuncA", &scaler::callNativeFuncA, "callNativeFuncA");
m.def("callNativeCallFuncA", &scaler::callNativeCallFuncA, "callNativeCallFuncA");
m.def("callFuncAThroughDLOpen", &scaler::callFuncAThroughDLOpen, "callFuncAThroughDLOpen");
}