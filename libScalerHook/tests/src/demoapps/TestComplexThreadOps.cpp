#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <cstdio>
#include <util/hook/install.h>
#include <util/tool/StringTool.h>

//=========================================================================
// Thread Functions
//=========================================================================

// Declaration of thread condition variable
pthread_cond_t cond0 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

// declaring mutex
pthread_mutex_t lock0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

unsigned int turn = 0;

void *testThread1(void *data) {
    while (true) {
        pthread_mutex_lock(&lock0);
        while (turn % 3 != 0 && turn < 6) {
            pthread_cond_wait(&cond0, &lock0);
        }

        if (turn < 6) {
            int randNum = rand() % 100 + 1;
            void *memAddr = malloc(1024);
            fprintf(stderr, "A Rand:%d %p\n", randNum, memAddr);
            ++turn;

            pthread_mutex_lock(&lock1);
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&lock1);

            pthread_mutex_unlock(&lock0);
        } else {
            //Notify next thread to exit

            pthread_mutex_lock(&lock1);
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&lock1);

            pthread_mutex_unlock(&lock0);
            break;
        }
    }
    return nullptr;
}

void *testThread2(void *data) {
    while (true) {
        pthread_mutex_lock(&lock1);
        while (turn % 3 != 1 && turn < 6) {
            pthread_cond_wait(&cond1, &lock1);
        }
        if (turn < 6) {
            int randNum = rand() % 100 + 1;
            void *memAddr = malloc(1024);
            fprintf(stderr, "B Rand:%d %p\n", randNum, memAddr);

            ++turn;

            pthread_mutex_lock(&lock2);
            pthread_cond_signal(&cond2);
            pthread_mutex_unlock(&lock2);

            pthread_mutex_unlock(&lock1);
        } else {
            //Notify next thread to exit

            pthread_mutex_lock(&lock2);
            pthread_cond_signal(&cond2);
            pthread_mutex_unlock(&lock2);

            pthread_mutex_unlock(&lock1);
            break;
        }
    }
    return nullptr;
}

void *testThread3(void *data) {
    while (true) {
        pthread_mutex_lock(&lock2);
        while (turn % 3 != 2 && turn < 6) {
            pthread_cond_wait(&cond2, &lock2);
        }
        if (turn < 6) {
            int randNum = rand() % 100 + 1;
            void *memAddr = malloc(1024);
            fprintf(stderr, "C Rand:%d %p\n", randNum, memAddr);
            ++turn;

            pthread_mutex_lock(&lock0);
            pthread_cond_signal(&cond0);
            pthread_mutex_unlock(&lock0);

            pthread_mutex_unlock(&lock2);
        } else {
            //Notify next thread to exit

            pthread_mutex_lock(&lock0);
            pthread_cond_signal(&cond0);
            pthread_mutex_unlock(&lock0);

            pthread_mutex_unlock(&lock2);
            break;
        }
    }
    return nullptr;
}

//=========================================================================
// Debugger loop
//=========================================================================

volatile int DEBUGGER_WAIT = 1;

void test_continue() {
    DEBUGGER_WAIT = 0;
}


int main(int argc, char *argv[]) {
    install([](std::string fileName, std::string funcName) -> bool {
        //todo: User should be able to specify name here. Since they can change filename

        if (scaler::strContains(fileName, "/ld-")) {
            return false;
        } else if (scaler::strContains(fileName, "/liblibScalerHook-HookManual")) {
            return false;
        } else if (scaler::strContains(fileName, "/libstdc++")) {
            return false;
        } else if (scaler::strContains(fileName, "/libdl-")) {
            return false;
        } else if (scaler::strContains(fileName, "/libScalerHook-demoapps-ComplexThreadop")) {
            return true;
        } else {
            fprintf(stderr, "%s:%s\n", fileName.c_str(), funcName.c_str());
            return false;
        }

    });

    pthread_t thread1, thread2, thread3;
    int iret1, iret2, iret3;

    /* Create independent threads each of which will execute function */
    iret1 = pthread_create(&thread1, NULL, testThread1, NULL);
    iret2 = pthread_create(&thread2, NULL, testThread2, NULL);
    iret3 = pthread_create(&thread3, NULL, testThread3, NULL);

    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

//    while (DEBUGGER_WAIT) {
    //Let gdb break
    //raise(SIGTRAP);
//    }

    exit(0);
    return 0;
}

