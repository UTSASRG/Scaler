
include(GNUInstallDirs)

add_library(Testlib-FuncCall SHARED ../../lib/FunctionCall/FuncWithDiffParms.cpp
        ../../lib/FunctionCall/TenThousandFunc.cpp)
target_include_directories(Testlib-FuncCall PUBLIC ../../lib/FunctionCall/include)
target_compile_options(Testlib-FuncCall PRIVATE ${TEST_FLAGS} -Werror)
target_link_options(Testlib-FuncCall PUBLIC "-zlazy")
## A library that calls Testlib-FuncCall

add_library(Testlib-CallFuncCall SHARED ../../lib/CallFunctionCall/CallFunctionCall.cpp)
target_include_directories(Testlib-CallFuncCall PUBLIC ../../lib/CallFunctionCall/include)
target_link_libraries(Testlib-CallFuncCall Testlib-FuncCall)
target_compile_options(Testlib-CallFuncCall PRIVATE ${TEST_FLAGS} -Werror)

install(
        TARGETS Testlib-FuncCall
        EXPORT Testlib-FuncCall-Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
        TARGETS Testlib-CallFuncCall
        EXPORT Testlib-CallFuncCall-Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(DEFINED SKBUILD)
    # prevent an unused variable warning
    set(ignoreMe "${SKBUILD}")

    # call pybind11-config to obtain the root of the cmake package (pybind11 should be install via pip).
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -m pybind11 --cmakedir
            OUTPUT_VARIABLE pybind11_ROOT_RAW)
    string(STRIP ${pybind11_ROOT_RAW} pybind11_ROOT)
    find_package(pybind11)

    pybind11_add_module(_TestNativeLibraryCall MODULE "TestNativeLibraryCall/TestNativeLibraryCall.cpp")

    target_link_libraries(_TestNativeLibraryCall PRIVATE Testlib-FuncCall Testlib-CallFuncCall)
    target_link_libraries(_TestNativeLibraryCall PRIVATE Testlib-FuncCall Testlib-CallFuncCall)
    target_compile_definitions(_TestNativeLibraryCall PRIVATE CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}")

    # Installing the extension module to the root of the package
    install(TARGETS _TestNativeLibraryCall DESTINATION .)

    # The extension module must load the hello library as a dependency when the
    # extension module is loaded. The easiest way to locate the hello library is
    # via RPATH. Absolute RPATHs are possible, but they make the resulting
    # binaries not redistributable to other Python installations (conda is broke,
    # wheel reuse is broke, and more!).
    #
    # Placing the hello library in the package and using relative RPATHs that
    # doesn't point outside of the package means that the built package is
    # relocatable. This allows for safe binary redistribution.
    if(APPLE)
        set_target_properties(
                _TestNativeLibraryCall PROPERTIES INSTALL_RPATH "@loader_path/${CMAKE_INSTALL_LIBDIR}")
    else()
        set_target_properties(_TestNativeLibraryCall PROPERTIES INSTALL_RPATH
                "$ORIGIN/${CMAKE_INSTALL_LIBDIR}")
    endif()

endif()