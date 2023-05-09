#ifndef SCALER_PTHREADPROXY_H
#define SCALER_PTHREADPROXY_H

#include<pthread.h>
extern "C" {

int pthread_create_proxy(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);

}
#endif //SCALER_PTHREADPROXY_H
