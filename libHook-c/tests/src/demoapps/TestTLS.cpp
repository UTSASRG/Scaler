/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
//#include <iostream>
#include <pthread.h>
#include <cstdio>

//#include <thread>
//#include <chrono>

using namespace std;

//void *print_message_function(void *ptr);


# define THREAD_SELF \
  ({ pthread_t *__self;                                                      \
     asm ("mov %%fs:%c1,%0" : "=r" (__self)                                      \
          : "i" (0x10));                       \
     __self;})


int main() {
//    pthread_t;
    pthread_t pt1 = pthread_self();
    printf("pt1=%lu\n", pt1);
    printf("pt1=%lu\n", (long unsigned int) THREAD_SELF);

//    pthread_t thread1, thread2;
//    char *message1 = "Thread 1";
//    char *message2 = "Thread 2";
//    int iret1, iret2;
//
//    /* Create independent threads each of which will execute function */
//
//    iret1 = pthread_create(&thread1, NULL, print_message_function, (void *) message1);
//    iret2 = pthread_create(&thread2, NULL, print_message_function, (void *) message2);
//
//    /* Wait till threads are complete before main continues. Unless we  */
//    /* wait we run the risk of executing an exit which will terminate   */
//    /* the process and all threads before the threads have completed.   */
//
//    printf("thread1 id=%lu\n", thread1);
//    printf("thread2 id=%lu\n", thread2);
//
//    pthread_join(thread1, NULL);
//    pthread_join(thread2, NULL);
//
//    printf("Thread 1 returns: %d\n", iret1);
//    printf("Thread 2 returns: %d\n", iret2);

//    scaler::ExtFuncCallHookAsm *libPltHook = scaler::ExtFuncCallHookAsm::getInst();
//    libPltHook->saveCommonFuncID();
//    libPltHook->saveAllSymbolId();
}


//
//void *print_message_function(void *ptr) {
//
//    char *message;
//    message = (char *) ptr;
//    pthread_self();
//    pthread_t thread;
//}