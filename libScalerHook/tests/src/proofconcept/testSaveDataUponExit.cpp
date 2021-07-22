#include <cstdio>
#include <pthread.h>

class DeconstructionTesting {
    int data;

    DeconstructionTesting() {
        printf("%p called Constructor\n", pthread_self());
    }

    ~DeconstructionTesting() {
        printf("%p called Deconstructor, data=%d\n", pthread_self(), data);
    }
};

thread_local DeconstructionTesting deconstructionTesting;

void *threadA(void *) {

    for (int i = 0; i < 1468; ++i) {
        deconstructionTesting.data = i;
    }

}

void *threadB(void *) {
    for (int i = 0; i < 1597; ++i) {
        deconstructionTesting.data = i;
    }
}

int main() {
    pthread_t tidA, tidB;

    /* Create independent threads each of which will execute function */
    printf("%p: Create tidA\n", pthread_self());
    pthread_create(&tidA, NULL, threadA, nullptr);
    printf("%p: Create tidB\n", pthread_self());
    pthread_create(&tidB, NULL, threadB, nullptr);

    /* Wait till threads are complete before main continues. Unless we  */
    pthread_join(tidA, nullptr);
    pthread_join(tidB, nullptr);
    printf("%p: TidA finished\n", pthread_self());
    printf("%p: TidB finished\n", pthread_self());

}
