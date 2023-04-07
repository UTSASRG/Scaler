#include <unistd.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>

void sleep3Seconds(){
    printf("Sleep3Seconds Start\n");
    struct timespec rem={0,0}, req = {
            30, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep3Seconds End\n");
}

void sleep1Second(){
    printf("Sleep1Second Start\n");
    struct timespec rem={0,0}, req = {
            10, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep1Seconds End\n");
}


void sleep5Seconds(){
    printf("Sleep5Seconds Start\n");
    struct timespec rem={0,0}, req = {
            50, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep5Seconds End\n");
}

void sleep10Seconds(){
    printf("Sleep10Seconds Start\n");
    struct timespec rem={0,0}, req = {
            10, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep10Seconds End\n");
}

void sleep15Seconds(){
    printf("Sleep15Seconds Start\n");
    struct timespec rem={0,0}, req = {
            15, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep15Seconds End\n");
}

