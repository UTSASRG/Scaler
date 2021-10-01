#include <cstdio>
//#include <sys/prctl.h>
#include <thread>

using namespace std;

using namespace std;

void *print_message_function(void *ptr);


#define _GNU_SOURCE 1

#include <asm/ldt.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <immintrin.h>

void * install_tls() {
    void *addr = mmap(0, 4096, PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (syscall(SYS_arch_prctl,ARCH_SET_GS, addr) < 0)
        return NULL;

    return addr;
}

void freeTLS() {
    void *addr;
    syscall(SYS_arch_prctl,ARCH_GET_GS, &addr);
    munmap(addr, 4096);
}

bool set_tls_value(int idx, unsigned long val) {
    if (idx < 0 || idx >= 4096/8) {
        return false;
    }
    asm volatile(
    "movq %0, %%gs:(%1)\n"
    :
    : "q"((void *)val), "q"(8ll * idx));
    return true;
}


unsigned long get_tls_value(int idx) {
    long long rc;
    if (idx < 0 || idx >= 4096/8) {
        return 0;
    }
    asm volatile(
    "movq %%fs:(%1), %0\n"
    : "=q"(rc)
    : "q"(8ll * idx));
    return rc;
}


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

void *print_message_function(void *ptr) {
    char *message;
    message = (char *) ptr;
    auto fs = _readfsbase_u64();
//    auto gs = _readgsbase_u64();
    printf("%lu\n", get_tls_value(1));
}
