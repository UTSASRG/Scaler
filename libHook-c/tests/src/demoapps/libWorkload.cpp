/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <unistd.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>

void sleep30Seconds(){
    printf("Sleep30Seconds Start\n");
    struct timespec rem={0,0}, req = {
            30, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep30Seconds End\n");
}

void sleep50Seconds(){
    printf("Sleep50Second Start\n");
    struct timespec rem={0,0}, req = {
            50, 0
    };
    int rlt=nanosleep(&req,&rem);
    if(rlt!=0){
        printf("Error: %s",strerror(errno));
        exit(-1);
    }
    printf("Sleep50Seconds End\n");
}


void sleep5Seconds(){
    printf("Sleep5Seconds Start\n");
    struct timespec rem={0,0}, req = {
            5, 0
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

