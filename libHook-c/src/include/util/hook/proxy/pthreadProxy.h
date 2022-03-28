#ifndef SCALER_PTHREADPROXY_H
#define SCALER_PTHREADPROXY_H

#include<pthread.h>



int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
//void pthread_exit(void *__retval);

#endif //SCALER_PTHREADPROXY_H
