# This cmake file builds
project(Tests)

message("Including gtest ${gtest_SOURCE_DIR}")

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-access-control")

# This library is used to test external funciton invocation
add_library(FuncCallTest SHARED libtest/src/installTest.cpp
        libtest/src/TenThousandFunc.cpp)
target_include_directories(FuncCallTest PUBLIC libtest/header)

# PltHook is used to compare results
add_library(KuBoPltHook ../libs/plthook/plthook_elf.c)
target_include_directories(KuBoPltHook PUBLIC ../libs/plthook)

# Build unittest examples for libhook
add_executable(TestLibHook
        TestPMParser.cpp
        TestExtFuncCallHook.cpp
        TestFileTool.cpp
        TestException.cpp
        TestELFParser.cpp)
target_include_directories(TestLibHook PUBLIC ../libScalerHook/headers libtest/header)
target_link_libraries(TestLibHook scalerhook KuBoPltHook FuncCallTest gtest gtest_main dl)
target_link_options(TestLibHook PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/unittests/libScalerHook/oldexperiments/mylinker.ld")


add_executable(TestExtSymbol TestFuncCall.cpp)
target_include_directories(TestExtSymbol PUBLIC ../libScalerHook/headers libtest/header)
target_link_libraries(TestExtSymbol scalerhook KuBoPltHook FuncCallTest dl)


add_library(InstallTest SHARED libInstalltest/src/installTest.cpp)
target_include_directories(InstallTest PUBLIC libInstalltest/header)
target_link_libraries(InstallTest PUBLIC FuncCallTest)

add_executable(TestInstall TestInstall.cpp)
target_include_directories(TestInstall PUBLIC libtest/header)
target_link_libraries(TestInstall scalerhook FuncCallTest InstallTest dl)
target_link_options(TestInstall PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/unittests/libScalerHook/oldexperiments/mylinker.ld")

add_executable(TestMultiThreadFuncCall TestMultiThreadFuncCall.cpp)
target_include_directories(TestMultiThreadFuncCall PUBLIC libtest/header)
target_link_libraries(TestMultiThreadFuncCall scalerhook FuncCallTest InstallTest dl)


add_executable(TestMultiThreadFuncCallPthread TestMultiThreadFuncCallPthread.cpp)
target_include_directories(TestMultiThreadFuncCallPthread PUBLIC libtest/header)
target_link_libraries(TestMultiThreadFuncCallPthread scalerhook FuncCallTest InstallTest dl)

add_executable(TestHookEverything TestHookEverything.cpp)
target_include_directories(TestHookEverything PUBLIC libtest/header)
target_link_libraries(TestHookEverything scalerhook FuncCallTest InstallTest dl)


add_executable(TestNestingCall TestNestingCall.cpp)
target_include_directories(TestNestingCall PUBLIC libtest/header)
target_link_libraries(TestNestingCall scalerhook FuncCallTest InstallTest dl)