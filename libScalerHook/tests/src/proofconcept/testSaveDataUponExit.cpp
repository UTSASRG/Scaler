#include <cstdio>
#include <pthread.h>

class DeconstructionTesting {
public:
    int data;

    DeconstructionTesting() {
        printf("%lu called Constructor\n", pthread_self());
    }

    ~DeconstructionTesting() {
        printf("%lu called Deconstructor, data=%d\n", pthread_self(), data);
    }
};

thread_local DeconstructionTesting deconstructionTesting;

void *threadA(void *) {

    for (int i = 0; i < 1468; ++i) {
        deconstructionTesting.data = i;
    }
    return nullptr;
}

void *threadB(void *) {
    for (int i = 0; i < 1597; ++i) {
        deconstructionTesting.data = i;
    }
    return nullptr;
}

int main() {
    pthread_t tidA, tidB;

    /* Creat%luindependent threads each of which will execute function */
    printf("%lu: Create tidA\n", pthread_self());
    pthread_create(&tidA, NULL, threadA, nullptr);
    printf("%lu: Create tidB\n", pthread_self());
    pthread_create(&tidB, NULL, threadB, nullptr);

    /* Wait till threads are complete before main continues. Unless we  */
    pthread_join(tidA, nullptr);
    pthread_join(tidB, nullptr);
    printf("%lu: TidA finished\n", pthread_self());
    printf("%lu: TidB finished\n", pthread_self());

}
