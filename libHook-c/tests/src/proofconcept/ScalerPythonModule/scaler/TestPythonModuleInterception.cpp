#include <iostream>
#include <pthread.h>
#include <thread>
#include <chrono>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <frameobject.h>



static PyInterpreterState* inter() {
    return PyInterpreterState_Main();
}

//PyObject* frameInterceptor(PyThreadState* ts, PyFrameObject* f, int throwflag) {
//    printf("Frame intercepted\n");
//    return _PyEval_EvalFrameDefault(ts, f, throwflag);
//}

#ifndef SCALER_PYTHON_ABI_COMPATIBILITY
# error "SCALER_PYTHON_ABI_COMPATIBILITY is undefined. Please build using the provided cmake build system."
#endif

#if SCALER_PYTHON_ABI_COMPATIBILITY == 311
PyObject* frameInterceptor(PyThreadState* ts, _PyInterpreterFrame* f, int throwflag) {
    printf("Frame intercepted\n");
    return _PyEval_EvalFrameDefault(ts, f, throwflag);
}
#else
# error "Python interpreter mismatch. Please check compatibility list"
#endif
static PyObject *hello(PyObject *self, PyObject *args) {
    printf("Hello this is Scaler python module\n");
    return Py_None;
}


static PyObject *install(PyObject *self, PyObject *args) {

    auto prev = _PyInterpreterState_GetEvalFrameFunc(inter());
    _PyInterpreterState_SetEvalFrameFunc(inter(), frameInterceptor);
    if (prev == frameInterceptor) {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


static PyMethodDef scalerMethodsDef[] = {
        {"install", install, METH_VARARGS, "Install Scaler."},
        {"hello",   hello, METH_VARARGS, "Install Scaler."},
        {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyModuleDef scalerModuleDef = {
        PyModuleDef_HEAD_INIT,
        "scaler",
        "Scaler python module.",
        -1,
        scalerMethodsDef,
};


static PyObject *scalerException;
PyMODINIT_FUNC PyInit_scaler() {
//    printf("Here1\n");
    PyObject *module= PyModule_Create(&scalerModuleDef);
    if (module == NULL) {
        return NULL;
    }
    return module;
}