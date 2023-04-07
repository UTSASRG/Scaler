#include <pthread.h>
#include <libWorkload.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>

void * pthread1Run(void *){
    sleep5Seconds();
}

void* pthread2Run(void*){
    sleep3Seconds();
}

void* pthread3Run(void*){
    sleep1Second();
}


int main(){
    pthread_t pthread1;
    pthread_create(&pthread1,nullptr,pthread1Run,nullptr);
    struct timespec rem={0,0}, req = {
            10, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    pthread_t pthread2;
    pthread_create(&pthread2,nullptr,pthread2Run,nullptr);
    struct timespec rem1={0,0}, req1 = {
            10, 0
    };
    int rlt1=nanosleep(&req1,&rem1);
    if(rlt1!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    pthread_t pthread3;
    pthread_create(&pthread3,nullptr,pthread3Run,nullptr);

    pthread_join(pthread1,nullptr);
    pthread_join(pthread2,nullptr);
    pthread_join(pthread3,nullptr);

}