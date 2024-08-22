/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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