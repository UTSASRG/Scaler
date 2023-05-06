#include <cstdio>
#include <dlfcn.h>
#include "DLInterceptor.h"

#define PRINT_INFO_LOG true
#define PRINT_DBG_LOG false
#define PRINT_ERR_LOG true
#if PRINT_DBG_LOG

// Print a single log string
#define DBG_LOG(str) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,"%s\n",str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,fmt,__VA_ARGS__); fprintf(stdout,"\n")
// Print a single error string

#else

// Print a single log string
#define DBG_LOG(str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...)
// Print a single error string

#endif

#if PRINT_ERR_LOG


#define ERR_LOG(str) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code) fprintf(stderr,"ERR: %s:%d ErrCode=%d  ",__FILE__,__LINE__,code); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__);fprintf(stderr,"\n")

#else

#define ERR_LOG(str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...)

#endif


#define fatalError(errMsg) \
    ERR_LOG(errMsg);        \
    exit(-1);

#define fatalErrorS(errFmt, ...) \
    ERR_LOGS(errFmt,__VA_ARGS__);            \
    exit(-1);


static DLAddrContainer *dlAddrContainer = nullptr;


void *dlopen(const char *__file, int __mode) __THROWNL {
    fprintf(stderr, "dlopen start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    void *rlt = dlAddrContainer->dlopenAddr(__file, __mode);
    fprintf(stderr, "dlopen end\n");
    return rlt;
}

int dlclose(void *__handle) __THROWNL  {
    fprintf(stderr, "dlclose start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    int rlt = dlAddrContainer->dlcloseAddr(__handle);
    fprintf(stderr, "dlclose end\n");
    return rlt;
}

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym(void *__restrict __handle, const char *__restrict __name) __THROW  {
    fprintf(stderr, "dlsym start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    void *rlt = dlAddrContainer->dlsymAddr(__handle, __name);
    fprintf(stderr, "dlsym end\n");
    return rlt;
}

#ifdef __USE_GNU

void *dlmopen(Lmid_t __nsid, const char *__file, int __mode) __THROWNL {
    fprintf(stderr, "dlmopen start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    void *rlt = dlAddrContainer->dlmopenAddr(__nsid, __file,__mode);
    fprintf(stderr, "dlmopen end\n");
    return rlt;
}

void *dlvsym(void *__restrict __handle,
             const char *__restrict __name,
             const char *__restrict __version)
__THROW  {
    fprintf(stderr, "dlvsym start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    void *rlt = dlAddrContainer->dlvsymAddr(__handle, __name,__version);
    fprintf(stderr, "dlvsym end\n");
    return rlt;
}

#endif

char *dlerror(void) __THROW {
    fprintf(stderr, "dlerror start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    char *rlt = dlAddrContainer->dlerrorAddr();
    fprintf(stderr, "dlerror end\n");
    return rlt;
}


#ifdef __USE_GNU

int dladdr(const void *__address, Dl_info *__info) __THROW  {
    fprintf(stderr, "dladdr start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    int rlt=dlAddrContainer->dladdrAddr(__address,__info);
    fprintf(stderr, "dladdr end\n");
    return rlt;
}

int dladdr1(const void *__address, Dl_info *__info, void **__extra_info, int __flags) __THROW  {
    fprintf(stderr, "dladdr1 start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    int rlt=dlAddrContainer->dladdr1Addr(__address,__info,__extra_info,__flags);
    fprintf(stderr, "dladdr1 end\n");
    return rlt;
}

int dlinfo(void *__restrict __handle, int __request, void *__restrict __arg) __THROW {
    fprintf(stderr, "dlinfo start\n");
    if (dlAddrContainer == nullptr) {
        fatalError("dlAddrContainer not initialized")
    }
    int rlt=dlAddrContainer->dlInfoAddr(__handle,__request,__arg);

    fprintf(stderr, "dlinfo end\n");
}

#endif


void initialize(DLAddrContainer &dlac) {
    //todo:Memory leak

    if (dlAddrContainer == NULL) {
        dlAddrContainer = (DLAddrContainer *) malloc(sizeof(DLAddrContainer));
        if (!dlAddrContainer) {
            fatalError("Cannot allocate new memory for dlAddrContainer");
        }
        dlAddrContainer->dlopenAddr = dlac.dlopenAddr;
        dlAddrContainer->dlcloseAddr = dlac.dlcloseAddr;
        dlAddrContainer->dlsymAddr = dlac.dlsymAddr;
        dlAddrContainer->dlmopenAddr = dlac.dlmopenAddr;
        dlAddrContainer->dlvsymAddr = dlac.dlvsymAddr;
        dlAddrContainer->dlerrorAddr = dlac.dlerrorAddr;
#ifdef __USE_GNU
        dlAddrContainer->dladdrAddr = dlac.dladdrAddr;
        dlAddrContainer->dladdr1Addr = dlac.dladdr1Addr;
        dlAddrContainer->dlInfoAddr = dlac.dlInfoAddr;
#endif /* __USE_GNU */
    }
}

