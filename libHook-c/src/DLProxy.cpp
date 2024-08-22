/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <util/hook/proxy/DLProxy.h>
#include <cstdlib>
#include "util/tool/Logging.h"
#include "util/datastructure/MemoryHeapList.h"
#include "util/hook/HookContext.h"
#include "util/tool/StringTool.h"
#include <util/hook/ExtFuncCallHook.h>

namespace scaler {
    void *dlopen_proxy(const char *__file, int __mode) __THROWNL {
        INFO_LOGS("dlopen_proxy for %s %p", __file,pthread_self());
        void *rlt = dlopen(__file, __mode);
        if (rlt) {
            //Successfully opened the library
            //DBG_LOGS("Installing on to open %s", __file);
            scaler::ExtFuncCallHook *inst = scaler::ExtFuncCallHook::getInst();

            if (!inst) {
                ERR_LOG("Scaler hook failed because Scaler is not initialized yet.");
                return rlt;
            }

            //Actual installation
            inst->installDlOpen();

            return rlt;
        } else {
            DBG_LOGS("dlopen for %s failed. Scaler will not install on this file.", __file);
            return rlt;
        }


    }

    template<typename DLSYM_FUNCTION_POINTER, typename... DLSYM_EXTRA_ARGS>
    void *searchNextSymbol(void *returnAddr, const char *__restrict __name, DLSYM_FUNCTION_POINTER dlsymPtr,
                           DLSYM_EXTRA_ARGS... args) {
        scaler::ExtFuncCallHook *instance = scaler::ExtFuncCallHook::getInst();
        void *rltFuncAddress = nullptr;
        const scaler::PMEntry &pmEntry = *(instance->pmParser.findPmEntryIdByAddr(returnAddr));
        const scaler::FileEntry &fileEntry = instance->pmParser.getFileEntry(pmEntry.globalFileId);

        //Search symbol in subsequent libraries
        ssize_t fileEntryArraySize = instance->pmParser.getFileEntryArraySize();
        for (int curFileId = 0; curFileId < fileEntryArraySize; ++curFileId) {
            const scaler::FileEntry &fileEntry = instance->pmParser.getFileEntry(curFileId);
            INFO_LOGS("Search %s in %s", __name, fileEntry.filePath.c_str());
            if (!fileEntry.valid || curFileId == pmEntry.globalFileId || !strEndsWith(fileEntry.filePath, ".so")) {
                continue;
            }
            void *libraryHandle = dlopen(fileEntry.filePath.c_str(), RTLD_LAZY);
            if (libraryHandle == nullptr) {
#ifndef NDEBUG
                if (fileEntry.valid) { fatalError(
                            "File is considered valid by Scaler, but dlopen failed on existing library %s. This should be impossible.")
                }
#endif
                continue;
            }
            rltFuncAddress = dlsymPtr(libraryHandle, __name, args...);
            if (rltFuncAddress != nullptr) {
                DBG_LOGS("Symbol %s not found in %s", __name, fileEntry.filePath.c_str());
                break;
            }
        }

        return rltFuncAddress;
    }

    //TODO: Python may also has dynamic loading

    typedef void *(dlsym_t)(void *__restrict __handle, const char *__restrict __name) __THROW __nonnull ((2));

    typedef void *(dlvsym_t)(void *__restrict __handle, const char *__restrict __name,
                             const char *__restrict __version) __THROW __nonnull ((2, 3));

    /* Find the run-time address in the shared object HANDLE refers to
    of the symbol called NAME.  */
    void *dlsym_proxy(void *__restrict __handle, const char *__restrict __name) __THROW {
        scaler::ExtFuncCallHook *instance = scaler::ExtFuncCallHook::getInst();
        HookContext *curHookContextPtr = curContext;
        void *realFuncAddr = nullptr;
        if (__handle == RTLD_NEXT) {
            void *retAddr = (void *) curHookContextPtr->hookTuple[curHookContextPtr->indexPosi].callerAddr;
            assert(retAddr != nullptr);
            realFuncAddr = searchNextSymbol<dlsym_t>(retAddr, __name, dlsym);
        } else {
            realFuncAddr = dlsym(__handle, __name);
        }
        void *scalerReturnAddr = nullptr;
        instance->installDlSym(__name, realFuncAddr, scalerReturnAddr);
        return scalerReturnAddr;
    }

#ifdef __USE_GNU

    void *dlmopen_proxy(Lmid_t __nsid, const char *__file, int __mode) __THROW {
        //todo: Handle correct private namespace
        INFO_LOG("dlmopen Interception Start");
        INFO_LOG("dlmopen Interception End");
        void *rlt = dlmopen(__nsid, __file, __mode);

        if (rlt) {
            //Successfully opened the library
            //DBG_LOGS("Installing on to open %s", __file);
            scaler::ExtFuncCallHook *inst = scaler::ExtFuncCallHook::getInst();

            if (!inst) {
                ERR_LOG("Scaler hook failed because Scaler is not initialized yet.");
                return rlt;
            }

            //Actual installation
            inst->installDlOpen();

            return rlt;
        } else {
            DBG_LOGS("dlmopen for %s failed. Scaler will not install on this file.", __file);
            return rlt;
        }

    }

    int dlclose_proxy(void *__handle) __THROWNL  {
        scaler::ExtFuncCallHook *inst = scaler::ExtFuncCallHook::getInst();
        inst->parseRealFileId();
        INFO_LOG("Parse realfileId on dlclose");
        int rlt = dlclose(__handle);
        return rlt;
    }


    void *dlvsym_proxy(void *__restrict __handle,
                       const char *__restrict __name,
                       const char *__restrict __version)
    __THROW {
        scaler::ExtFuncCallHook *instance = scaler::ExtFuncCallHook::getInst();
        void *realFuncAddr = nullptr;
        if (__handle == RTLD_NEXT) {
            void *retAddr = __builtin_return_address(0);
            assert(retAddr != nullptr);
            realFuncAddr = searchNextSymbol<dlvsym_t, const char *__restrict>(retAddr, __name, dlvsym, __version);
        } else {
            realFuncAddr = dlvsym(__handle, __name, __version);
        }
        void *scalerReturnAddr = nullptr;
        instance->installDlSym(__name, realFuncAddr, scalerReturnAddr);
        return scalerReturnAddr;
    }
}
#endif

