/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <pybind11/pybind11.h>
#include <cstdio>

#include <iostream>
#include <pthread.h>
#include <thread>
#include <chrono>


#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <frameobject.h>
#include <ceval.h>
#include <util/datastructure/MemoryHeapList.h>
#include <util/hook/ExtFuncCallHook.h>
#include <util/hook/LogicalClock.h>
#include <util/tool/Timer.h>
#include <type/ExtSymInfo.h>
#include <type/PyCodeExtra.h>
#include <util/hook/HookContext.h>
#include <map>
#include <pystate.h>
#include <pybind11/pybind11.h>
#include <type/PyCodeExtra.h>

#ifndef SCALER_PYTHON_ABI_COMPATIBILITY
#error "SCALER_PYTHON_ABI_COMPATIBILITY is undefined. Please build using the provided cmake build system."
#endif

extern __thread HookContext *curContext;

namespace scaler {
    int SCALER_PYCODEEXTRA_INDEX = -1;
    const int SCALER_PY_PACKAGE_LEVEL = 1;//How many level of packages should Scaler treat as packages. 1 means if hello.a and hello.b will be treated as the same package hello
    MemoryHeapList<PyCodeExtra> *pyCodeExtraHeap = nullptr;//todo:memory leak

    std::map<std::string, FileID> *fileNameFileIdMap = nullptr;
    scaler::ExtFuncCallHook *inst = nullptr;

    void hello() {
        printf("libHook-c Ver");
    }

    inline PyCodeExtra *getCodeExtra(PyFrameObject *f) {
        PyCodeExtra *curCodeExtra = nullptr;
        int pyCodeExtraRlt = _PyCode_GetExtra((PyObject *) f->f_code, SCALER_PYCODEEXTRA_INDEX,
                                              (void **) &curCodeExtra);
        assert(pyCodeExtraRlt != -1);
        return curCodeExtra;
    }

    inline PyCodeExtra *ensureScalerIsInstalled(PyFrameObject *f) {
        PyCodeExtra *curCodeExtra = getCodeExtra(f);
        if (!curCodeExtra) {
            //PyCodeObject is null. Initialize pycode object.
            curCodeExtra = pyCodeExtraHeap->malloc();
            new(curCodeExtra) PyCodeExtra();
            assert(curCodeExtra->pyModuleRecArrMap.getSize() == 0);
            int setRlt = _PyCode_SetExtra((PyObject *) f->f_code, SCALER_PYCODEEXTRA_INDEX, (void *) curCodeExtra);
            assert(setRlt == 0);

            std::string pyNameVar;
            try {
                pyNameVar = pybind11::cast<std::string>(pybind11::globals()["__name__"]);
            } catch (pybind11::error_already_set &cat) {
                pyNameVar = "";
            }

            ssize_t subStrEndLoc = 0;
            for (int i = 0; i < SCALER_PY_PACKAGE_LEVEL; ++i) {
                subStrEndLoc = pyNameVar.find('.', subStrEndLoc);
                if (subStrEndLoc == -1) {
                    subStrEndLoc = pyNameVar.length();
                    break;
                }
            }
            std::string pyModuleName = pyNameVar.substr(0, subStrEndLoc);

            auto fileIdIter = fileNameFileIdMap[0].find(pyModuleName);
            ssize_t newFileId = -1;
            if (fileIdIter == fileNameFileIdMap[0].end()) {
                //Allocate a new file Id
                pthread_mutex_lock(&inst->dynamicLoadingLock);
                newFileId = inst->elfImgInfoMap.getSize();
                ELFImgInfo *curElfImgInfo = inst->elfImgInfoMap.pushBack();
                fileNameFileIdMap[0][pyModuleName] = newFileId;
                pthread_mutex_unlock(&inst->dynamicLoadingLock);
                INFO_LOGS("Allocated a new fileId %zd for module %s", newFileId, pyModuleName.c_str());
            } else {
                //Found fileId
                newFileId = fileIdIter->second;
            }

            //Record new recId in code extra
            curCodeExtra->pyModuleFileId = newFileId;
        }

        return curCodeExtra;
    }

    inline ssize_t getFrameNumber(PyFrameObject *f) {
        ssize_t fNumber = 1; //One frame has a depth value of 1. Two frame has depth of 2.
        PyFrameObject *previousFrame = f->f_back;
        while (previousFrame != nullptr) {
            ++fNumber;
            previousFrame = previousFrame->f_back;
        }
        return fNumber;
    }

    inline RecTuple &getRecTuple(PyCodeExtra *prevCodeExtra, PyCodeExtra *curCodeExtra, PyFrameObject *curFrame) {
        ssize_t originalSize = curCodeExtra->pyModuleRecArrMap.getSize();
        //Make sure there are enough space  to store this entry
        for (ssize_t i = originalSize - 1; i <= prevCodeExtra->pyModuleFileId; ++i) {
            ssize_t *newRecId = curCodeExtra->pyModuleRecArrMap.pushBack();
            *newRecId = -1;//Indicate this invocation relation has no allocated entry in recording array yet.
        }
        ssize_t &curRecId = curCodeExtra->pyModuleRecArrMap[prevCodeExtra->pyModuleFileId];

        HookContext *curContextPtr = curContext;
        if (curRecId == -1) {
            //Allocate new recording entry and record the ID.
            //Allocate a new recEntry in recTuple
            curRecId = curContextPtr->ldArr.getSize();
            RecTuple *curRecTuple = curContextPtr->ldArr.pushBack();
            INFO_LOGS("Current function name is %s", PyUnicode_AsUTF8(curFrame->f_code->co_name));
            INFO_LOGS("Allocate a new record Id. Id updated to: %zd", curRecId);
            return *curRecTuple;
        } else {
            //INFO_LOGS("Existing record Id is: %zd",curRecId);
            return curContext->ldArr[curRecId];
        }
    }

#ifndef SCALER_PYTHON_ABI_COMPATIBILITY
#error "SCALER_PYTHON_ABI_COMPATIBILITY is undefined. Please build using the provided cmake build system."
#endif
#if SCALER_PYTHON_ABI_COMPATIBILITY == 311
    PyObject* frameInterceptor(PyThreadState* ts, _PyInterpreterFrame* f, int throwflag) {
        printf("Frame intercepted\n");
        return _PyEval_EvalFrameDefault(ts,f,throwflag);
    }
#elif SCALER_PYTHON_ABI_COMPATIBILITY == 309
    PyObject* frameInterceptor(PyThreadState* ts, PyFrameObject* f, int throwflag) {
        HookContext *curContextPtr=curContext;
        if(curContextPtr==nullptr){
            INFO_LOGS("curContextPtr==nullptr pthread_self=%p",pthread_self());
        }
        assert(curContextPtr!=nullptr);
        uint64_t preLogicalClockCycle = calcCurrentLogicalClock(curContextPtr->cachedWallClockSnapshot,
                                                                curContextPtr->cachedLogicalClock, curContextPtr->cachedThreadNum);
        PyObject* rlt=_PyEval_EvalFrameDefault(ts, f, throwflag);
        uint64_t postLogicalClockCycle = calcCurrentLogicalClock(curContextPtr->cachedWallClockSnapshot,
                                                                curContextPtr->cachedLogicalClock, curContextPtr->cachedThreadNum);
        uint64_t clockCyclesDuration = (int64_t) (postLogicalClockCycle-preLogicalClockCycle);

        PyFrameObject* previousFrame=f->f_back;
        RecTuple* curRecTuple=nullptr;
        if(previousFrame){
            PyCodeExtra* prevCodeExtra = ensureScalerIsInstalled(previousFrame);
            //Check if the current module is a new module or not. We can check this by requesting code extra.
            PyCodeExtra* curCodeExtra = ensureScalerIsInstalled(f);
            curRecTuple=&getRecTuple(prevCodeExtra,curCodeExtra,f);
            curRecTuple->totalClockCycles+=clockCyclesDuration;
        }else{
            printf("Detected exits");
        }

        return rlt;
    }
#else
#error "Python interpreter mismatch. Please check compatibility list"
#endif

    PyFrameObject *findRootFrame() {
        PyFrameObject *previousFrame = PyEval_GetFrame();
        while (previousFrame->f_back != nullptr) {
            printf("Python frame at installation time %p", previousFrame);
            previousFrame = previousFrame->f_back;
        }
        return previousFrame;
    }

    static bool installationStatus = false;

    bool install() {
        if (installationStatus) {
            ERR_LOG("Scaler has already been installed");
            return false;
        }
        installationStatus = true;
        //Install C first. Make sure thread context is initialized.

        INFO_LOGS("libHook-c Ver %s", CMAKE_SCALERRUN_VERSION);
        INFO_LOGS("Main thread id is %lu", pthread_self());
        INFO_LOG("Installed through python interpreter");

        std::stringstream ss;
        char *pathFromEnv = getenv("SCALER_OUTPUT_PATH");
        if (pathFromEnv == NULL) {
            ss << "/tmp";
        } else {
            ss << pathFromEnv;
        }
        ss << "/scalerdata_" << getunixtimestampms();
        INFO_LOGS("Folder name is %s", ss.str().c_str());


        inst = scaler::ExtFuncCallHook::getInst(ss.str().c_str());
        inst->install();
        HookContext *curContextPtr = curContext;
        assert(curContextPtr != nullptr);
        curContextPtr->threadCreatorFileId = 0;
        curContextPtr->isMainThread = true;

        assert(curContextPtr != nullptr);
#if SCALER_PYTHON_ABI_COMPATIBILITY == 309
        ssize_t requestedIndex=_PyEval_RequestCodeExtraIndex(freePyCodeExtra);
#else
        ssize_t requestedIndex = PyUnstable_Eval_RequestCodeExtraIndex(freePyCodeExtra);
#endif

        SCALER_PYCODEEXTRA_INDEX = requestedIndex;

        printf("Installing scaler python module");
        pyCodeExtraHeap = new MemoryHeapList<PyCodeExtra>();
        fileNameFileIdMap = new std::map<std::string, FileID>();

        PyFrameObject *curFrame = PyEval_GetFrame();

        ssize_t frameNumber = getFrameNumber(curFrame);
        INFO_LOGS("Current frame nubmer is %zd", frameNumber);

        if (frameNumber > MAX_CALL_DEPTH) {
            //Skip exceeded frames
            for (ssize_t i = 0; i < MAX_CALL_DEPTH - frameNumber; ++i) {
                curFrame = curFrame->f_back;
            }
            frameNumber = MAX_CALL_DEPTH;
        }
        INFO_LOGS("Current frame number is %zd", frameNumber);

//        for (ssize_t i = frameNumber - 1; i >= 0; --i) {
//            DBG_LOGS("Python frame at installation time %p",curFrame);
//            //Initialize pycode object for this frame
//            DBG_LOGS("Initial install frame %zd",i);
//
//            PyCodeExtra* curCodeExtra = ensureScalerIsInstalled(curFrame);
//            curContextPtr->hookTuple[i].callerAddr=0;
//            curContextPtr->hookTuple[i].id.fileId=curCodeExtra->pyModuleFileId;
//            curFrame=curFrame->f_back;
//        }

        auto prev = _PyInterpreterState_GetEvalFrameFunc(PyInterpreterState_Main());
        //_PyInterpreterState_SetEvalFrameFunc(PyInterpreterState_Main(), frameInterceptor);
        if (prev == frameInterceptor) {
            return false;
        }
        return true;
    }

}  // namespace hello


PYBIND11_MODULE(_scaler, m) {
    m.doc() = "Example module";
    m.def("hello", &scaler::hello, "Prints Scaler Version");
    m.def("install", &scaler::install, "Install scaler");
}