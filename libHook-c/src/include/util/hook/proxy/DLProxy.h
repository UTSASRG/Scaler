#ifndef SCALER_DLPROXY_H
#define SCALER_DLPROXY_H

#include <dlfcn.h>


void* dlopen_proxy(const char *__file, int __mode) __THROWNL;

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym_proxy(void *__restrict __handle, const char *__restrict __name) __THROW __nonnull ((2));

#ifdef __USE_GNU
void *dlmopen_proxy(Lmid_t __nsid, const char *__file, int __mode) __THROWNL;

void *dlvsym_proxy(void *__restrict __handle,
             const char *__restrict __name,
             const char *__restrict __version)
__THROW __nonnull ((2, 3));
#endif



#endif //SCALER_DLPROXY_H
