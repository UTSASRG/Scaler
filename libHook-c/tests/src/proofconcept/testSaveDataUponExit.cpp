/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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
