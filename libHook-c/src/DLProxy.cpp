#include <util/hook/proxy/DLProxy.h>
#include "util/tool/Logging.h"

void *dlopen_proxy(const char *__file, int __mode) __THROWNL {
    INFO_LOG("Dlopen Interception Start");
    void *rlt = dlopen(__file, __mode);
    INFO_LOG("Dlopen Interception End");
    return rlt;
}

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym_proxy(void *__restrict __handle, const char *__restrict __name) __THROW {
    INFO_LOG("Dlsym Interception Start");
    void *rlt = dlsym(__handle, __name);
    INFO_LOG("Dlsym Interception End");
    return rlt;
}

#ifdef __USE_GNU

void *dlmopen_proxy(Lmid_t __nsid, const char *__file, int __mode) __THROW {
    INFO_LOG("dlmopen Interception Start");
    INFO_LOG("dlmopen Interception End");
}

void *dlvsym_proxy(void *__restrict __handle,
                   const char *__restrict __name,
                   const char *__restrict __version)
__THROW {
    INFO_LOG("dlvsym Interception Start");

    INFO_LOG("dlvsym Interception End");
}

#endif

