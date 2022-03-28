#ifndef SCALER_LIBCPROXY_H
#define SCALER_LIBCPROXY_H

extern "C" {

typedef int (*main_fn_t)(int, char **, char **);

int __libc_start_main(main_fn_t, int, char **, void (*)(), void (*)(), void (*)(), void *)
__attribute__((weak, alias("doubletake_libc_start_main")));

int doubletake_libc_start_main(main_fn_t main_fn, int argc, char **argv, void (*init)(), void (*fini)(),
                               void (*rtld_fini)(), void *stack_end);

}
#endif //SCALER_LIBCPROXY_H
