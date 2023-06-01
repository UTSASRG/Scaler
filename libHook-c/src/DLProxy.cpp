#include <util/hook/proxy/DLProxy.h>
#include <cstdlib>
#include "util/tool/Logging.h"
#include "util/datastructure/MemoryHeapList.h"
#include <util/hook/ExtFuncCallHook.h>

void *dlopen_proxy(const char *__file, int __mode) __THROWNL {
    void *rlt = dlopen(__file, __mode);

    if(rlt){
        //Successfully opened the library
        DBG_LOGS("Installing on to open %s",__file);
        scaler::ExtFuncCallHook* inst=scaler::ExtFuncCallHook::getInst();
        if(!inst){
            ERR_LOG("Scaler hook failed because Scaler is not initialized yet.");
            return rlt;
        }

        inst->installDlOpen();

        return rlt;
    }else{
        DBG_LOGS("dlopen for %s failed. Scaler will not install on this file.",__file);
        return rlt;
    }


}


//TODO: Match

/* Find the run-time address in the shared object HANDLE refers to
   of the symbol called NAME.  */
void *dlsym_proxy(void *__restrict __handle, const char *__restrict __name) __THROW {
    //todo: Consider multithreading
    INFO_LOG("Dlsym Interception Start");
    void *realFuncAddr = dlsym(__handle, __name);
    INFO_LOGS("funcAddr=%p",realFuncAddr);

    //TODO: Lock protect
    void* retAddr=nullptr;
    if(!scaler::ExtFuncCallHook::getInst()->installDlSym(realFuncAddr, retAddr)){
        ERR_LOGS("Failed to hook on %s",__name);
        return realFuncAddr;
    }else{
        INFO_LOG("Dlsym Interception End");
        return retAddr;
    }


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

