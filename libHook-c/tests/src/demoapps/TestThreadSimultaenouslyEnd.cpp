/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <pthread.h>
#include <libWorkload.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>

void * pthread1Run(void *){
    sleep10Seconds();
}

void* pthread2Run(void*){
    sleep10Seconds();
}

void* pthread3Run(void*){
    sleep10Seconds();
}


int main(){
    pthread_t pthread1;
    pthread_create(&pthread1,nullptr,pthread1Run,nullptr);

    pthread_t pthread2;
    pthread_create(&pthread2,nullptr,pthread2Run,nullptr);

    pthread_t pthread3;
    pthread_create(&pthread3,nullptr,pthread3Run,nullptr);

    pthread_join(pthread1,nullptr);
    pthread_join(pthread2,nullptr);
    pthread_join(pthread3,nullptr);
}