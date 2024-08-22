
#The following code will find python libraries in specified location
#Variable SCALER_PYTHON_ABI_COMPATIBILITY is used to indicate the minimum python ABI compatible
set(Python_FIND_STRATEGY LOCATION)
#set(Python_ROOT_DIR "/home/steven/usr/miniconda3/envs/ScalerDev37")
#set(Python_ROOT_DIR "/home/steven/usr/miniconda3/envs/ScalerDev38")
#set(Python_ROOT_DIR "/home/steven/usr/miniconda3/envs/ScalerDev39")
#set(Python_ROOT_DIR "/home/steven/usr/miniconda3/envs/ScalerDev310")
#set(SCALER_PYTHON_ABI_COMPATIBILITY 311) #This variable indicates the minimum version that is compatible with this API

set(Python_ROOT_DIR /home/steven/usr/miniconda3/envs/ScalerDev311)
list(APPEND CMAKE_MODULE_PATH "${Python_ROOT_DIR}/lib/python3.11/site-packages/skbuild/resources/cmake/")
set(SCALER_PYTHON_ABI_COMPATIBILITY 311) #This variable indicates the minimum version that is compatible with this API

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
message(STATUS "Python Module Library to Use: ${Python_EXECUTABLE} SCALER_PYTHON_ABI_COMPATIBILITY: ${SCALER_PYTHON_ABI_COMPATIBILITY}")

#find_package(PythonExtensions REQUIRED)

#add_library(ScalerHook-proof-PythonModuleInterception SHARED scaler/TestPythonModuleInterception.cpp)
#target_link_libraries(ScalerHook-proof-PythonModuleInterception PUBLIC pthread Python::Module)
#target_compile_definitions(ScalerHook-proof-PythonModuleInterception PRIVATE SCALER_PYTHON_ABpyproject.toml7I_COMPATIBILITY=${SCALER_PYTHON_ABI_COMPATIBILITY})
#python_extension_module(ScalerHook-proof-PythonModuleInterception)
#install(TARGETS ScalerHook-proof-PythonModuleInterception LIBRARY DESTINATION scaler)