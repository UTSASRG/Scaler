# Compile LibPltHook
project(ScalerHook-Tests)

enable_testing()
find_package(GTest REQUIRED)

add_compile_options("-fno-access-control")

# ============================================================
# Test libs
# ============================================================

## A library that does a range of function call
add_library(libScalerHook-testlib-FuncCall SHARED lib/FunctionCall/FuncWithDiffParms.cpp
        lib/FunctionCall/TenThousandFunc.cpp)
target_include_directories(libScalerHook-testlib-FuncCall PUBLIC lib/FunctionCall/include)

## A library that calls libScalerHook-testlib-FuncCall

add_library(libScalerHook-testlib-CallFuncCall SHARED lib/CallFunctionCall/CallFunctionCall.cpp)
target_include_directories(libScalerHook-testlib-CallFuncCall PUBLIC lib/CallFunctionCall/include)
target_link_libraries(libScalerHook-testlib-CallFuncCall libScalerHook-testlib-FuncCall)

## PltHook is used to compare results
add_library(KuBoPltHook lib/plthook/plthook_elf.c src/proofconcept/test/testPltHookEnum.cpp)
target_include_directories(KuBoPltHook PUBLIC lib/plthook)

# ============================================================
# Unit Test
# ============================================================

add_executable(libScalerHook-unittests
        src/unittests/TestPMParser.cpp
        src/unittests/TestExtFuncCallHook.cpp
        src/unittests/TestFileTool.cpp
        src/unittests/TestException.cpp
        src/unittests/TestELFParser.cpp)
target_link_libraries(libScalerHook-unittests PUBLIC libScalerHook-HookManual libScalerHook-testlib-FuncCall KuBoPltHook gtest gtest_main dl pthread)
target_link_options(libScalerHook-unittests PUBLIC "LINKER:SHELL: -T ${ScalerHook-Tests_SOURCE_DIR}/linkerscripts/mylinker.ld")

# ============================================================
# Demo Application
# ============================================================

option(libScalerHook-demoapps_AUTO_INSTALL_HOOK "If ON, then libScalerHook-HookAuto will be linked. Otherwise libScalerHook-HookManual will be linked" ON)

if(libScalerHook-demoapps_AUTO_INSTALL_HOOK)

endif(libScalerHook-demoapps_AUTO_INSTALL_HOOK)


    add_executable(libScalerHook-demoapps-FuncCall src/demoapps/TestFuncCall.cpp)
target_link_libraries(libScalerHook-demoapps-FuncCall PUBLIC libScalerHook-HookManual libScalerHook-testlib-FuncCall
        libScalerHook-testlib-CallFuncCall KuBoPltHook dl)
#
#
#add_library(InstallTest SHARED libInstalltest/src/installTest.cpp)
#target_include_directories(InstallTest PUBLIC libInstalltest/header)
#target_link_libraries(InstallTest PUBLIC FuncCallTest)
#
#add_executable(TestInstall TestInstall.cpp)
#target_include_directories(TestInstall PUBLIC libtest/header)
#target_link_libraries(TestInstall scalerhook FuncCallTest InstallTest dl)
#target_link_options(TestInstall PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/unittests/libScalerHook/oldexperiments/mylinker.ld")
#
#add_executable(TestMultiThreadFuncCall TestMultiThreadFuncCall.cpp)
#target_include_directories(TestMultiThreadFuncCall PUBLIC libtest/header)
#target_link_libraries(TestMultiThreadFuncCall scalerhook FuncCallTest InstallTest dl)
#
#
#add_executable(TestMultiThreadFuncCallPthread TestMultiThreadFuncCallPthread.cpp)
#target_include_directories(TestMultiThreadFuncCallPthread PUBLIC libtest/header)
#target_link_libraries(TestMultiThreadFuncCallPthread scalerhook FuncCallTest InstallTest dl)
#
#add_executable(TestHookEverything TestHookEverything.cpp)
#target_include_directories(TestHookEverything PUBLIC libtest/header)
#target_link_libraries(TestHookEverything scalerhook FuncCallTest InstallTest dl)
#
#
#add_executable(TestNestingCall TestNestingCall.cpp)
#target_include_directories(TestNestingCall PUBLIC libtest/header)
#target_link_libraries(TestNestingCall scalerhook FuncCallTest InstallTest dl)