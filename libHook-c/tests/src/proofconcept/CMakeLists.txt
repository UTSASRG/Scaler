cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -ldl --verbose -znow -fno-omit-frame-pointer -O3 -std=c++11")
set(CMAKE_VERBOSE_MAKEFILE on)


include_directories(includes)


# Compile libOneMilionFunc
add_library(libOneMilionFunc SHARED milionFuncLibrary.cpp)


#target_link_libraries(test1 mylib dl PltHookLib)

add_executable(TestFuncInvocation testFuncInvocation.cpp)
target_link_libraries(TestFuncInvocation dl)

add_executable(TestASMCallC testASMCallFunc.cpp)

add_executable(TestExtFunctionInvocation testExtFuncInvocation.cpp)
target_link_libraries(TestExtFunctionInvocation Testlib-FuncCall  dl)



add_executable(TestAddr2Relative testParsingAddr2Relative.cpp)
target_link_libraries(TestAddr2Relative Testlib-FuncCall dl)


add_executable(TestFindSection testFindSection.cpp)

# This example will not work

add_library(DLInterceptor SHARED DLInterceptor.cpp)
target_link_libraries(DLInterceptor PRIVATE dl)

add_library(DlGrabber SHARED DlGrabber.cpp)
target_link_libraries(DlGrabber PRIVATE dl)

add_executable(TestDL TestDL.cpp)
target_link_libraries(TestDL ScalerHook-HookAutoAsm-C dl)
target_include_directories(TestDL PUBLIC /home/steven/Projects/Scaler/libHook-c/tests/lib/FunctionCall/include)


