#ifndef SCALER_ASSEMBLYTOOL_H
#define SCALER_ASSEMBLYTOOL_H

# define THREAD_SELF \
  ({ pthread_t *__self; \
     asm ("mov %%fs:%c1,%0" : "=r" (__self) \
          : "i" (0x10));\
     __self;})
#endif //SCALER_ASSEMBLYTOOL_H
