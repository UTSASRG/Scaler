#include <thread>
#include <pthread.h>
#include <linux/unistd.h>
#include <asm/ldt.h>
#include <sys/syscall.h>
#include <sys/syscall.h>     /* Definition of SYS_* constants */
#include <unistd.h>
# include <asm/ldt.h>        /* Definition of struct user_desc */



using namespace std;


void threadA() {
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        printf("ThreadA print....%d\n", i);
    }
}

void threadB() {
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        printf("ThreadB print....%d\n", i);
    }
}

void threadC() {
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        printf("ThreadC print....%d\n", i);
    }
}

int main() {
    //https://reverseengineering.stackexchange.com/questions/14397/how-are-addresses-calculated-from-the-values-in-x86-segment-registers
//    void* __self;
//    asm ("mov %%fs:%c1,%0" : "=r" (__self)                                      \
//          : "i" (0x10));
//
//    void* __selfPtr;
//    asm ("lea %%fs:%c1,%0" : "=r" (__selfPtr)                                      \
//          : "i" (0x10));
//    pthread_self();
//    printf("%p\n", __self);
//    printf("%p\n", pthread_self());
//    printf("%p\n", __selfPtr);
//
//    printf("");
//    perror("whyfailed");
//

    printf("Testing application start\n");

    std::this_thread::sleep_for (std::chrono::seconds(5));

    std::thread thread1(threadA);
    std::thread thread2(threadB);
    std::thread thread3(threadC);
    thread1.join();
    thread2.join();
    thread3.join();
    printf("Thread execution complete\n");
    return 0;
}