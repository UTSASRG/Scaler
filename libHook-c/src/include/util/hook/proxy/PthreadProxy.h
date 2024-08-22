/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_PTHREADPROXY_H
#define SCALER_PTHREADPROXY_H

#include<pthread.h>
namespace scaler{
    int pthread_create_proxy(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
    //void pthread_exit(void *__retval);
//    int pthread_cond_timedwait_proxy(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime);
//    int pthread_cond_wait_proxy(pthread_cond_t * cond, pthread_mutex_t * mutex);
//    int pthread_join_proxy(pthread_t thread, void **value_ptr);
};


#endif //SCALER_PTHREADPROXY_H
