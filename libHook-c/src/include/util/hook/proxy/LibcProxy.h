#ifndef SCALER_LIBCPROXY_H
#define SCALER_LIBCPROXY_H

extern "C" {

typedef int (*main_fn_t)(int, char **, char **);
#ifndef MANUAL_INSTALL

void exit(int __status) __attribute__ ((noreturn));


#endif
}
#endif //SCALER_LIBCPROXY_H
