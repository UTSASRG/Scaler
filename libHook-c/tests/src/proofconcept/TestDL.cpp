#include <cstdio>
#include <cstdlib>
#include <FuncWithDiffParms.h>
#include <dlfcn.h>

typedef void (*FuncAType)();

//This example will not work
int main() {
    printf("TestDL");

    void *libHandle = dlopen("/home/steven/Projects/Scaler/cmake-build-release/libHook-c/tests/libTestlib-FuncCall.so",
                             RTLD_LAZY);
    if (!libHandle) {
        fprintf(stderr,"Cannot open libTestlib-FuncCall.so\n");
    }
    auto funcAPtr = (FuncAType) dlsym(libHandle, "funcA");
    if(!funcAPtr){
        fprintf(stderr,"Cannot find funcA\n");
    }
    funcAPtr();
    return 0;
}