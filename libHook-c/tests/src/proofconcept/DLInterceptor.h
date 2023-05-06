//#include <cstdio>
//#include <cstdlib>

#include <cstdio>
#include <cstdio>
#include <cstdlib>
#include <features.h>

__BEGIN_DECLS

typedef void *(*dlopen_t)(const char *__file, int __mode) __THROWNL;

typedef int (*dlclose_t)(void *__handle) __THROWNL __nonnull ((1));

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
typedef void *(*dlsym_t)(void *__restrict __handle,
                         const char *__restrict __name) __THROW __nonnull ((2));

#ifdef __USE_GNU
typedef void *(*dlmopen_t)(Lmid_t __nsid, const char *__file, int __mode) __THROWNL;

typedef void *(*dlvsym_t)(void *__restrict __handle,
                          const char *__restrict __name,
                          const char *__restrict __version)
        __THROW __nonnull ((2, 3));
#endif

typedef char *(*dlerror_t)(void) __THROW;


#ifdef __USE_GNU

typedef int (*dladdr_t)(const void *__address, Dl_info *__info) __THROW __nonnull ((2));

typedef int (*dladdr1_t)(const void *__address, Dl_info *__info, void **__extra_info, int __flags) __THROW __nonnull (
        (2));

typedef int (*dlinfo_t)(void *__restrict __handle, int __request, void *__restrict __arg) __THROW __nonnull ((1, 3));

#endif /* __USE_GNU */


__END_DECLS

struct DLAddrContainer {
    dlopen_t dlopenAddr;
    dlclose_t dlcloseAddr;
    dlsym_t dlsymAddr;
    dlmopen_t dlmopenAddr;
    dlvsym_t dlvsymAddr;
    dlerror_t dlerrorAddr;
#ifdef __USE_GNU
    dladdr_t dladdrAddr;
    dladdr1_t dladdr1Addr;
    dlinfo_t dlInfoAddr;
#endif /* __USE_GNU */
};


void* dlopen(const char *__file, int __mode) __THROWNL;

int dlclose(void *__handle) __THROWNL __nonnull ((1));

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym(void *__restrict __handle, const char *__restrict __name) __THROW __nonnull ((2));

#ifdef __USE_GNU
void *dlmopen(Lmid_t __nsid, const char *__file, int __mode) __THROWNL;

void *dlvsym(void *__restrict __handle,
             const char *__restrict __name,
             const char *__restrict __version)
__THROW __nonnull ((2, 3));
#endif

char *dlerror(void) __THROW;


#ifdef __USE_GNU

int dladdr(const void *__address, Dl_info *__info) __THROW __nonnull ((2));

int dladdr1(const void *__address, Dl_info *__info, void **__extra_info, int __flags) __THROW __nonnull (
        (2));

int dlinfo(void *__restrict __handle, int __request, void *__restrict __arg) __THROW __nonnull ((1, 3));

#endif

void initialize(DLAddrContainer& dlac);