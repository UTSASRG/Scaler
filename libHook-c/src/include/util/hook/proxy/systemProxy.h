#ifndef SCALER_LIBCPROXY_H
#define SCALER_LIBCPROXY_H

#include <ctype.h>

typedef __pid_t (*fork_origt)(void);

__pid_t fork(void);

#endif //SCALER_LIBCPROXY_H
