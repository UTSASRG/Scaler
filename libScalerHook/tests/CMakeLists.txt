# Compile LibPltHook
project(ScalerHook-Tests)

enable_testing()
find_package(GTest REQUIRED)

add_compile_options("-fno-access-control" "--verbose" "-fPIC" "-fstack-protector-all")
add_link_options("-zlazy")
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
add_library(KuBoPltHook lib/plthook/plthook_elf.c)
target_include_directories(KuBoPltHook PUBLIC lib/plthook)

# ============================================================
# Unit Test
# ============================================================

add_executable(libScalerHook-unittests
        src/unittests/TestPMParser.cpp
        src/unittests/TestExtFuncCallHook.cpp
        src/unittests/TestFileTool.cpp
        src/unittests/TestMemTool.cpp
        src/unittests/TestException.cpp
        src/unittests/TestELFParser.cpp
        src/unittests/TestLog.cpp
        src/unittests/TestConfig.cpp
        src/unittests/TestTimer.cpp
        src/unittests/TestInvocationTree.cpp
        src/unittests/util/datastructure/TestList.cpp
        src/unittests/util/datastructure/TestStack.cpp
        src/unittests/util/datastructure/TestVector.cpp
        src/unittests/util/datastructure/TestHashmap.cpp src/unittests/util/datastructure/TestELFInfo.cpp)
target_link_libraries(libScalerHook-unittests PUBLIC ScalerHook-HookManualAsm libScalerHook-testlib-FuncCall KuBoPltHook gtest gtest_main dl pthread)
target_link_options(libScalerHook-unittests PUBLIC "LINKER:SHELL: -T ${ScalerHook-Tests_SOURCE_DIR}/linkerscripts/mylinker.ld")

# ============================================================
# Demo Application
# ============================================================

option(libScalerHook-demoapps_AUTO_INSTALL_HOOK "If ON, then ScalerHook-HookAuto will be linked. Otherwise ScalerHook-HookManualAsm will be linked" ON)

add_executable(libScalerHook-demoapps-FuncCall src/demoapps/TestFuncCall.cpp)
target_link_libraries(libScalerHook-demoapps-FuncCall PUBLIC ScalerHook-HookAuto libScalerHook-testlib-FuncCall
        libScalerHook-testlib-CallFuncCall KuBoPltHook dl)
target_compile_options(libScalerHook-demoapps-FuncCall PRIVATE "-fstack-protector" "-zlazy")

add_executable(libScalerHook-demoapps-HookEverything src/demoapps/TestHookEverything.cpp)
target_include_directories(libScalerHook-demoapps-HookEverything PUBLIC libtest/header)
target_link_libraries(libScalerHook-demoapps-HookEverything PUBLIC ScalerHook-HookAuto libScalerHook-testlib-FuncCall
        libScalerHook-testlib-CallFuncCall KuBoPltHook dl)

add_executable(libScalerHook-demoapps-FuncCallDelay src/demoapps/TestFuncCallDelay.cpp)
target_link_libraries(libScalerHook-demoapps-FuncCallDelay PUBLIC ScalerHook-HookAuto libScalerHook-testlib-FuncCall
        libScalerHook-testlib-CallFuncCall dl pthread)

add_executable(libScalerHook-demoapps-FuncNestedCall src/demoapps/TestNestedFuncCallPthread.cpp)
target_link_libraries(libScalerHook-demoapps-FuncNestedCall PUBLIC ScalerHook-HookAuto libScalerHook-testlib-FuncCall
        libScalerHook-testlib-CallFuncCall dl pthread)


add_executable(libScalerHook-demoapps-Pthread src/demoapps/TestPthread.cpp)
target_include_directories(libScalerHook-demoapps-Pthread PUBLIC libtest/header)
target_link_libraries(libScalerHook-demoapps-Pthread ScalerHook-HookAuto libScalerHook-testlib-FuncCall pthread dl)


add_executable(libScalerHook-demoapps-Threadlocal src/demoapps/TestThreadLocalAssembly.cpp)
target_include_directories(libScalerHook-demoapps-Threadlocal PUBLIC libtest/header)
target_link_libraries(libScalerHook-demoapps-Threadlocal pthread dl)
target_compile_options(libScalerHook-demoapps-Threadlocal PUBLIC "-mfsgsbase")

add_executable(libScalerHook-demoapps-StdPthread src/demoapps/TestStdThread.cpp)
target_include_directories(libScalerHook-demoapps-StdPthread PUBLIC libtest/header)
target_link_libraries(libScalerHook-demoapps-StdPthread ScalerHook-HookAuto libScalerHook-testlib-FuncCall libScalerHook-testlib-CallFuncCall pthread dl)


add_executable(libScalerHook-demoapps-ComplexThreadop src/demoapps/TestComplexThreadOps.cpp)
target_include_directories(libScalerHook-demoapps-ComplexThreadop PUBLIC libtest/header)
target_link_libraries(libScalerHook-demoapps-ComplexThreadop ScalerHook-HookAuto libScalerHook-testlib-FuncCall libScalerHook-testlib-CallFuncCall pthread dl)


add_executable(libScalerHook-demoapps-Semaphore src/demoapps/TestSemaphore.cpp)
target_include_directories(libScalerHook-demoapps-Semaphore PUBLIC)
target_link_libraries(libScalerHook-demoapps-Semaphore ScalerHook-HookAuto pthread dl)


add_executable(libScalerHook-demoapps-Signal src/demoapps/TestSingalHandler.cpp)
target_include_directories(libScalerHook-demoapps-Signal PUBLIC)
target_link_libraries(libScalerHook-demoapps-Signal ScalerHook-HookAuto pthread dl)

add_executable(libScalerHook-demoapps-InlineFunc src/demoapps/TestInlineFunction.cpp)
target_include_directories(libScalerHook-demoapps-InlineFunc PUBLIC)
target_compile_options(libScalerHook-demoapps-InlineFunc PUBLIC "-g")
target_link_libraries(libScalerHook-demoapps-InlineFunc ScalerHook-HookAuto)

add_executable(libScalerHook-demoapps-TestTLS src/demoapps/TestTLS.cpp)
target_include_directories(libScalerHook-demoapps-TestTLS PUBLIC)
target_compile_options(libScalerHook-demoapps-TestTLS PUBLIC "-g")
target_link_libraries(libScalerHook-demoapps-TestTLS pthread)

add_executable(libScalerHook-parsecapps-aget
        src/parsecapps/aget/Aget.c
        src/parsecapps/aget/Download.c
        src/parsecapps/aget/Head.c
        src/parsecapps/aget/main.c
        src/parsecapps/aget/Misc.c
        src/parsecapps/aget/Resume.c
        src/parsecapps/aget/Signal.c)
target_include_directories(libScalerHook-parsecapps-aget PUBLIC src/parsecapps/aget/include)
target_link_libraries(libScalerHook-parsecapps-aget ScalerHook-HookAuto-Brkpoint pthread dl)


add_executable(libScalerHook-parsecapps-dedup
        src/parsecapps/dedup/binheap.c
        src/parsecapps/dedup/decoder.c
        src/parsecapps/dedup/dedup.c
        src/parsecapps/dedup/encoder.c
        src/parsecapps/dedup/hashtable.c
        src/parsecapps/dedup/queue.c
        src/parsecapps/dedup/rabin.c
        src/parsecapps/dedup/tree.c
        src/parsecapps/dedup/util.c)
target_include_directories(libScalerHook-parsecapps-dedup PUBLIC src/parsecapps/dedup/include)
target_link_directories(libScalerHook-parsecapps-dedup PRIVATE src/parsecapps/dedup/lib)
target_link_libraries(libScalerHook-parsecapps-dedup  ScalerHook-HookAuto pthread dl
        libcrypto.a
        libssl.a
        libz.a)

add_executable(libScalerHook-parsecapps-blackscholes src/parsecapps/blackscholes/blackscholes-pthread.cpp)
target_compile_options(libScalerHook-parsecapps-blackscholes PRIVATE "-DENABLE_THREADS" "-DPARALLEL" "-DNCO=2" "-DN=960")
target_link_libraries(libScalerHook-parsecapps-blackscholes ScalerHook-HookAuto)


find_package(BZip2 REQUIRED)
add_executable(libScalerHook-parsecapps-pbzip2
        src/parsecapps/pbzip2/BZ2StreamScanner.cpp
        src/parsecapps/pbzip2/ErrorContext.cpp
        src/parsecapps/pbzip2/pbzip2.cpp)
target_include_directories(libScalerHook-parsecapps-pbzip2 PUBLIC src/parsecapps/pbzip2/include)
target_link_libraries(libScalerHook-parsecapps-pbzip2 ScalerHook-HookManualAsm pthread BZip2::BZip2)


#add_library(InstallTest SHARED libInstalltest/src/installTest.cpp)
#target_include_directories(InstallTest PUBLIC libInstalltest/header)
#target_link_libraries(InstallTest PUBLIC FuncCallTest)
#
#add_executable(TestInstall TestInstall.cpp)
#target_include_directories(TestInstall PUBLIC libtest/header)
#target_link_libraries(TestInstall scalerhook FuncCallTest InstallTest dl)
#target_link_options(TestInstall PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/unittests/libScalerHook/oldexperiments/mylinker.ld")

#
#add_executable(TestNestingCall TestNestingCall.cpp)
#target_include_directories(TestNestingCall PUBLIC libtest/header)
#target_link_libraries(TestNestingCall scalerhook FuncCallTest InstallTest dl)

# ============================================================
# Proof of concept
# ============================================================

add_executable(libScalerHook-proof-StackProtector src/proofconcept/TestStackOverflow.cpp)
target_compile_options(libScalerHook-proof-StackProtector PRIVATE "-fstack-protector")

add_executable(libScalerHook-proof-LTraceSimpleMain src/proofconcept/TestLtrace.cpp)
target_link_options(libScalerHook-proof-LTraceSimpleMain PRIVATE "-z" "lazy")

add_executable(libScalerHook-proof-HookEverything src/proofconcept/TestHookEverything.cpp)
target_link_options(libScalerHook-proof-HookEverything PRIVATE "-z" "lazy")
target_link_libraries(libScalerHook-proof-HookEverything libScalerHook-testlib-FuncCall libScalerHook-testlib-CallFuncCall)

add_executable(libScalerHook-proof-PthreadParam src/proofconcept/testPthreadParameter.cpp)
target_link_options(libScalerHook-proof-PthreadParam PRIVATE "-z" "now" "-O0") #Avoid plt lookup code.
target_link_libraries(libScalerHook-proof-PthreadParam pthread)

#add_executable(libScalerHook-proof-SwBreakpoint src/proofconcept/testSwBreakpoint.cpp)

add_executable(libScalerHook-proof-ManualBreakpoint src/proofconcept/testManualBreakpoint.cpp)

add_executable(libScalerHook-proof-ObjCopy src/proofconcept/TestObjCopy.cpp)

add_executable(libScalerHook-proof-PtraceReadProcMem src/proofconcept/testPtraceReadProcMemory.cpp)

add_executable(libScalerHook-proof-testMultithreadPrint src/proofconcept/testMultithreadPrint.cpp)
target_link_libraries(libScalerHook-proof-testMultithreadPrint pthread)
target_link_options(libScalerHook-proof-testMultithreadPrint PRIVATE "-z" "lazy" "-O0")

add_executable(libScalerHook-proof-SaveDataUponExit src/proofconcept/testSaveDataUponExit.cpp)
target_link_libraries(libScalerHook-proof-SaveDataUponExit pthread)


#add_executable(libScalerHook-demoapps src/proofconcept/testprog.cpp)
#target_link_libraries(DemoProg libTest PltHookLib dl)

##target_link_libraries(test1 mylib dl PltHookLib)
#
#add_executable(TestFuncInvocation test/testFuncInvocation.cpp)
#target_link_libraries(TestFuncInvocation dl)
#
#add_executable(TestASMCallC test/testASMCallFunc.cpp)
#
#add_executable(TestExtFunctionInvocation test/testExtFuncInvocation.cpp)
#target_link_libraries(TestExtFunctionInvocation libTest dl)
#
#add_executable(TestJMPHandler test/testJmpHandler.cpp)
#target_link_libraries(TestJMPHandler libTest PltHookLib dl)
#
#add_executable(TestJMPHandlerInlineHook test/testJmpHandlerInlineHook.cpp)
#target_link_libraries(TestJMPHandlerInlineHook libTest PltHookLib dl)
#target_link_options(TestJMPHandlerInlineHook PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")
#
#get_target_property(MAIN_CFLAGS TestJMPHandlerInlineHook LINKER_OPTOINS)
## also see: COMPILE_DEFINITIONS INCLUDE_DIRECTORIES
#message("-- Target compiler flags are: ${MAIN_CFLAGS}")
#
#add_executable(TestAddr2Relative test/testParsingAddr2Relative.cpp)
#target_link_libraries(TestAddr2Relative libTest PltHookLib dl)
#
#
#add_executable(SimpleProgPlt test/simeProgPLT.cpp)
#target_link_libraries(SimpleProgPlt libTest)
#
#add_executable(TestFindSection test/testFindSection.cpp)
#
#add_executable(TestPLTInterception test/testPLTInterception.cpp)
#target_link_options(TestPLTInterception PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")
#
#
##add_executable(TestLocatingPLT test/testLocatingPLT.cpp)
##target_link_libraries(TestLocatingPLT libTest PltHookLib libPMParser dl)
##target_link_options(TestLocatingPLT PUBLIC "LINKER:SHELL: -T ${CMAKE_SOURCE_DIR}/mylinker.ld")
#
#
#add_executable(TestFindRdebug test/testFindRdebug.cpp)
#target_link_libraries(TestFindRdebug libTest dl)

