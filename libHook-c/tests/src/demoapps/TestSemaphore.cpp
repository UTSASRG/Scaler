/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <thread>

sem_t mutex;

void *thread(void *arg) {
    //wait
    printf("threadID=%lu sem_wait sID=%p\n", pthread_self(), &mutex);
    sem_wait(&mutex);
    printf("\nEntered..\n");

    //critical section
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    //signal
    printf("\nJust Exiting...\n");
    printf("threadID=%lu sem_post sID=%p\n", pthread_self(), &mutex);
    sem_post(&mutex);
    return nullptr;
}


int main() {
    sem_init(&mutex, 0, 1);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread, NULL);
    sleep(1);
    pthread_create(&t2, NULL, thread, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    sem_destroy(&mutex);
    return 0;
}