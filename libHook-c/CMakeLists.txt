#This file is part of Scaler.
#Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.
#
#@author: Steven (Jiaxun) Tang <jtang@umass.edu>

project(Hook VERSION 0.2.0)


set(SCALER_HOOK_COMPILATION_FLAGS "-O3" "-g")
# https://json.nlohmann.me/integration/cmake/

#Required to make RPATH work
include(GNUInstallDirs)

#Add all files for libscalerhook
set(HookSrc
        src/ExtFuncCallHook.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/StringTool.cpp
        src/HookContext.cpp
        src/SystemProxy.cpp
        src/PthreadProxy.cpp
        src/ExtSymInfo.cpp
        src/ELFImgInfo.cpp
        src/HookHandlers.cpp
        src/DLProxy.cpp
        )

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(ScalerHook-HookManualAsm-C SHARED
        ${HookSrc} src/LibcProxy.cpp)
target_include_directories(ScalerHook-HookManualAsm-C PUBLIC src/include lib/inireader)
target_link_libraries(ScalerHook-HookManualAsm-C PUBLIC pthread dl)
target_compile_options(ScalerHook-HookManualAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookManualAsm-C PRIVATE MANUAL_INSTALL CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")

install(TARGETS ScalerHook-HookManualAsm-C
    EXPORT ScalerHook-HookManualAsm-C-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(ScalerHook-HookAutoAsm-C SHARED src/LibcProxy.cpp ${HookSrc})# ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(ScalerHook-HookAutoAsm-C PUBLIC src/include lib/inireader)# ${PROTO_SRC_DIR})
target_link_libraries(ScalerHook-HookAutoAsm-C PUBLIC pthread dl)
#target_link_options(ScalerHook-HookAutoAsm-C PRIVATE -static-libgcc -static-libstdc++) #Todo; Statically link libc is not advised. Change to re-compile in o
target_compile_options(ScalerHook-HookAutoAsm-C PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
target_compile_definitions(ScalerHook-HookAutoAsm-C PUBLIC CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}" NDEBUG)
install(
        TARGETS ScalerHook-HookAutoAsm-C
        EXPORT ScalerHook-HookAutoAsm-C-Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

find_package(CUDAToolkit)
if (${CUDAToolkit_FOUND})
    message(STATUS "CUDA found version: ${CUDAToolkit_VERSION}")
    add_library(ScalerHook-HookManualAsm-C-GPU SHARED
            ${HookSrc} src/LibcProxy.cpp src/CUDAProxy.cpp)
    target_include_directories(ScalerHook-HookManualAsm-C-GPU PUBLIC src/include lib/inireader)
    target_link_libraries(ScalerHook-HookManualAsm-C-GPU PUBLIC pthread dl CUDA::cuda_driver)
    target_compile_options(ScalerHook-HookManualAsm-C-GPU PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
    target_compile_definitions(ScalerHook-HookManualAsm-C-GPU PRIVATE MANUAL_INSTALL CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")

    install(
            TARGETS ScalerHook-HookManualAsm-C-GPU
            EXPORT ScalerHook-HookAutoAsm-C-Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    #ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
    add_library(ScalerHook-HookAutoAsm-C-GPU SHARED src/LibcProxy.cpp ${HookSrc} src/CUDAProxy.cpp)# ${PROTO_SRCS} ${GRPC_SRCS})
    target_include_directories(ScalerHook-HookAutoAsm-C-GPU PUBLIC src/include lib/inireader)# ${PROTO_SRC_DIR})
    target_link_libraries(ScalerHook-HookAutoAsm-C-GPU PUBLIC pthread dl CUDA::cuda_driver)
    target_link_options(ScalerHook-HookAutoAsm-C-GPU PRIVATE -static-libgcc -static-libstdc++) #Todo; Statically link libc is not advised. Change to re-compile in o
    target_compile_options(ScalerHook-HookAutoAsm-C-GPU PRIVATE ${SCALER_HOOK_COMPILATION_FLAGS})
    target_compile_definitions(ScalerHook-HookAutoAsm-C-GPU PUBLIC CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}" NDEBUG)

    install(
            TARGETS ScalerHook-HookAutoAsm-C-GPU
            EXPORT ScalerHook-HookAutoAsm-C-GPU-Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else ()

endif ()


#todo: Make Scaler importable through find_package
# We are using the SKBUILD variable, which is defined when scikit-build is
# running the CMake build, to control building the Python wrapper. This allows
# the C++ project to be installed, standalone, when using the standard CMake
# build flow.
if (DEFINED SKBUILD)
    # prevent an unused variable warning
    set(ignoreMe "${SKBUILD}")

    # call pybind11-config to obtain the root of the cmake package (pybind11 should be install via pip).
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -m pybind11 --cmakedir
            OUTPUT_VARIABLE pybind11_ROOT_RAW)
    string(STRIP ${pybind11_ROOT_RAW} pybind11_ROOT)
    find_package(pybind11)

    pybind11_add_module(_scaler MODULE "src/pythonroot/scaler/ScalerPythonModule.cpp")
    target_compile_definitions(_scaler PRIVATE SCALER_PYTHON_ABI_COMPATIBILITY=309)
    target_include_directories(_scaler PRIVATE src/pythonroot/scaler)
    target_compile_definitions(_scaler PRIVATE CMAKE_SCALERRUN_VERSION="${PROJECT_VERSION}")


    if (${CUDAToolkit_FOUND})
        target_link_libraries(_scaler PRIVATE ScalerHook-HookManualAsm-C-GPU)
    else ()
        target_link_libraries(_scaler PRIVATE ScalerHook-HookManualAsm-C)
    endif ()


    # Installing the extension module to the root of the package
    install(TARGETS _scaler DESTINATION .)

    # The extension module must load the hello library as a dependency when the
    # extension module is loaded. The easiest way to locate the hello library is
    # via RPATH. Absolute RPATHs are possible, but they make the resulting
    # binaries not redistributable to other Python installations (conda is broke,
    # wheel reuse is broke, and more!).
    #
    # Placing the hello library in the package and using relative RPATHs that
    # doesn't point outside of the package means that the built package is
    # relocatable. This allows for safe binary redistribution.
    if (APPLE)
        set_target_properties(_scaler PROPERTIES INSTALL_RPATH "@loader_path/${CMAKE_INSTALL_LIBDIR}")
    else ()
        set_target_properties(_scaler PROPERTIES INSTALL_RPATH "$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
    endif ()
endif ()


#Add tests
add_subdirectory(tests)