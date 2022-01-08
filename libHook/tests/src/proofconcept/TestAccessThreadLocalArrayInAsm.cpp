#include <iostream>
#include <pthread.h>
#include <thread>
#include <chrono>

using namespace std;

__thread static int localArray[100];
__thread static int localint;

extern "C" {
void printLocalArr() {
    for (int i = 0; i < 100; ++i) {
        printf("%d\n", localArray[i]);
    }
}
}

void *threadA(void *ptr) {
    for (int i = 0; i < 100; ++i) {
        localArray[i] = i;
    }
    printLocalArr();

}

//void *threadBasm(void *ptr) {
//    asm volatile(
//    "sub    $0x20,%rsp\n\t"
//    "mov    %rdi,-0x18(%rbp)\n\t"
//    "movq   $0x0,%r11\n\t"
//    "myloop: cmpq   $0x63,%r11\n\t"
//    "jg     finish\n\t"
//    "mov    %r11, @tpoff(localint)\n\t"
//    "addl   $0x1,-0x4(%rbp)\n\t"
//    "jmp    myloop\n\t"
//    "finish:nop\n\t"
//    );
//    printLocalArr();
//    return nullptr;
//}

int main() {
    pthread_t thread1;
    pthread_create(&thread1, nullptr, threadA, nullptr);
    pthread_join(thread1, nullptr);

}