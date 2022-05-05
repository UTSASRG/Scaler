#ifndef SCALER_LIBCPROXY_H
#define SCALER_LIBCPROXY_H

extern "C" {

typedef int (*main_fn_t)(int, char **, char **);
#ifndef MANUAL_INSTALL

int __libc_start_main(main_fn_t, int, char **, void (*)(), void (*)(), void (*)(), void *)
__attribute__((weak, alias("doubletake_libc_start_main")));


int doubletake_libc_start_main(main_fn_t main_fn, int argc, char **argv, void (*init)(), void (*fini)(),
                               void (*rtld_fini)(), void *stack_end);

typedef int (*exit_origt)(int __status) ;
void exit(int __status) __attribute__ ((noreturn));


#endif
}
#endif //SCALER_LIBCPROXY_H
