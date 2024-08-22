/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FuncWithDiffParms.h"
#include "CallFunctionCall.h"

using namespace std;



//=========================================================================
// Thread Functions
//=========================================================================

pthread_mutex_t count_mutex;

void *testThread1(void *data) {
    for (int i = 0; i < 10000; ++i) {
        pthread_mutex_lock(&count_mutex);
//        printf("thread1 executing\n");
        callLoopDelay(100);
        pthread_mutex_unlock(&count_mutex);
        pthread_yield();
    }
    return nullptr;
}

void *testThread2(void *data) {
    for (int i = 0; i < 50; ++i) {
        pthread_mutex_lock(&count_mutex);
//        printf("thread2 executing\n");
        callSleepDelay(100);
        pthread_mutex_unlock(&count_mutex);
        pthread_yield();
    }
    return nullptr;
}

int main() {
    pthread_mutex_lock(&count_mutex);
//        printf("thread2 executing\n");
    callSleepDelay(100);
    pthread_mutex_unlock(&count_mutex);
    pthread_yield();


    return 0;

    pthread_t thread1, thread2;
    int iret1, iret2;

    /* Create independent threads each of which will execute function */
    iret1 = pthread_create(&thread1, NULL, testThread1, NULL);
    iret2 = pthread_create(&thread2, NULL, testThread2, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    exit(0);
    return 0;
}
