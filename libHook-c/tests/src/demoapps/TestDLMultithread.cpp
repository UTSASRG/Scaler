/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <cstdio>
#include <cstdlib>
#include <FuncWithDiffParms.h>
#include <dlfcn.h>

typedef void (*FuncAType)();

void* dlopenThread1(void* args){
    printf("TestDL\n");

    void *libHandle = dlopen("/home/steven/Projects/Scaler-eurosys2024/cmake-build-debug/libHook-c/tests/libTestlib-FuncCall.so",
                             RTLD_LAZY);
    //Load the same library agagin, Scaler should not install again.
    libHandle = dlopen("/home/steven/Projects/Scaler-eurosys2024/cmake-build-debug/libHook-c/tests/libTestlib-FuncCall.so",
                    RTLD_LAZY);
    printf("LibHandle=%p",libHandle);

    if (!libHandle) {
        fprintf(stderr, "Cannot open libTestlib-FuncCall.so\n");
    } else {
        auto funcAPtr = (FuncAType) dlsym(libHandle, "funcA");
        if (!funcAPtr) {
            fprintf(stderr, "Cannot find funcA\n");
        } else {
            funcAPtr();
        }
    }
    return nullptr;
}
void* dlopenThread2(void* args){
    printf("TestDL\n");
    void *libHandle = dlopen("/home/steven/Projects/Scaler-eurosys2024/cmake-build-debug/libHook-c/tests/libTestlib-CallFuncCall.so",
                             RTLD_LAZY);
    printf("LibHandle=%p",libHandle);

    if (!libHandle) {
        fprintf(stderr, "Cannot open libTestlib-CallFuncCall.so\n");
    } else {
        auto funcAPtr = (FuncAType) dlsym(libHandle, "callFuncA");
        //dlsym twice, Scaler should only install once
        funcAPtr = (FuncAType) dlsym(libHandle, "callFuncA");
        if (!funcAPtr) {
            fprintf(stderr, "Cannot find callFuncA\n");
        } else {
            funcAPtr();
        }
    }
    return nullptr;
}

void* test(void* args){
    return nullptr;
}
//This example will not work
int main() {
    pthread_t thread1, thread2;
    std::string message1 = "Thread 1";
    std::string message2 = "Thread 2";
    int iret1, iret2;

    /* Create independent threads each of which will execute function */

    iret1 = pthread_create(&thread1, NULL, dlopenThread1, NULL);
    iret2 = pthread_create(&thread2, NULL, dlopenThread2, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */

    printf("thread1 id=%lu\n", thread1);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

//    printf("Thread 1 returns: %d\n", iret1);

//    scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
//    libPltHook->saveCommonFuncID();
//    libPltHook->saveAllSymbolId();
    return 0;
}