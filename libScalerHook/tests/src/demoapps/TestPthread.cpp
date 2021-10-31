#include <iostream>
#include <pthread.h>
#include <thread>
#include <chrono>
#include "../../../src/include/util/hook/install.h"
#include "../../../src/include/util/tool/StringTool.h"

using namespace std;

void *print_message_function(void *ptr);

int main() {
//    install([](std::string fileName, std::string funcName) -> bool {
//        //todo: User should be able to specify name here. Since they can change filename
//
//        if (fileName ==
//            "/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/tests/libScalerHook-demoapps-Pthread") {
//            fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
//            return true;
//        } else {
//            return false;
//        }
//
//    });

//    pthread_create((pthread_t *) 123, (pthread_attr_t *) 0x11, (void *(*)(void *)) (0x11),
//                   (void *) 0x11);
    pthread_t thread1, thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    int iret1, iret2;

    /* Create independent threads each of which will execute function */

    iret1 = pthread_create(&thread1, NULL, print_message_function, (void *) message1);
    iret2 = pthread_create(&thread2, NULL, print_message_function, (void *) message2);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */

    printf("thread1 id=%lu\n", thread1);
    printf("thread2 id=%lu\n", thread2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Thread 1 returns: %d\n", iret1);
    printf("Thread 2 returns: %d\n", iret2);

//    scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
//    libPltHook->saveCommonFuncID();
//    libPltHook->saveAllSymbolId();
}

thread_local char asdf;

void *print_message_function(void *ptr) {
    char *message;
    message = (char *) ptr;
    printf("%s \n", message);

    printf("%d %lu\n",&asdf, &asdf - (char*)pthread_self());
    return nullptr;

}