
# Compile LibPltHook
project(ScalerHook-Tests)

enable_testing()
find_package(GTest REQUIRED)

# ============================================================
# Test libs
# ============================================================

set(TEST_FLAGS "-fstack-protector-all")


## A library that does a range of function call
add_library(Testlib-FuncCall SHARED lib/FunctionCall/FuncWithDiffParms.cpp
        lib/FunctionCall/TenThousandFunc.cpp)
target_include_directories(Testlib-FuncCall PUBLIC lib/FunctionCall/include)
target_compile_options(Testlib-FuncCall PRIVATE ${TEST_FLAGS} -Werror)
target_link_options(Testlib-FuncCall PUBLIC "-zlazy")
## A library that calls Testlib-FuncCall

add_library(Testlib-CallFuncCall SHARED lib/CallFunctionCall/CallFunctionCall.cpp)
target_include_directories(Testlib-CallFuncCall PUBLIC lib/CallFunctionCall/include)
target_link_libraries(Testlib-CallFuncCall Testlib-FuncCall)
target_compile_options(Testlib-CallFuncCall PRIVATE ${TEST_FLAGS} -Werror)

## PltHook is used to compare results
add_library(KuBoPltHook lib/plthook/plthook_elf.c)
target_include_directories(KuBoPltHook PUBLIC lib/plthook)

# ============================================================
# Unit Test
# ============================================================
#include(${CMAKE_SOURCE_DIR}/proto/findThisGrpc.cmake)

add_executable(ScalerHook-unittests
        src/unittests/TestPMParser.cpp
        src/unittests/TestExtFuncCallHook.cpp
        src/unittests/TestFileTool.cpp
        src/unittests/TestMemTool.cpp
        src/unittests/TestMemoryHeapList.cpp
        src/unittests/TestMemoryHeapArray.cpp
        src/unittests/TestException.cpp
        src/unittests/TestELFParser.cpp
        src/unittests/TestLog.cpp
        src/unittests/TestConfig.cpp
        src/unittests/TestTimer.cpp
        src/unittests/TestInvocationTree.cpp
        src/unittests/util/datastructure/TestList.cpp
        src/unittests/util/datastructure/TestStack.cpp
        src/unittests/util/datastructure/TestVector.cpp
        src/unittests/util/datastructure/TestHashmap.cpp
        src/unittests/util/datastructure/TestELFInfo.cpp
        src/unittests/TestStr.cpp
        ${SCALER_PROTO_SRCS})
message("SCALER_PROTO_SRCS=1${SCLAER_GRPC_INCLUDE} 2${SCALER_PROTO_SRCS} ")
target_link_libraries(ScalerHook-unittests PUBLIC ScalerHook-HookManualAsm-C Testlib-FuncCall KuBoPltHook gtest gtest_main dl pthread)
target_compile_options(ScalerHook-unittests PRIVATE "-fno-access-control" ${TEST_FLAGS} -Werror)
target_include_directories(ScalerHook-unittests PRIVATE ${SCLAER_GRPC_INCLUDE})
message("include is ${SCLAER_GRPC++_LIBRARY}")

# ============================================================
# Demo Application
# ============================================================

option(ScalerHook-demoapps_AUTO_INSTALL_HOOK "If ON, then ScalerHook-HookAutoAsm-C will be linked. Otherwise ScalerHook-HookManualAsm-C will be linked" ON)

add_executable(ScalerHook-demoapps-Exceptions src/demoapps/TestExceptions.cpp)
target_link_libraries(ScalerHook-demoapps-Exceptions PUBLIC ScalerHook-HookAutoAsm-C Testlib-FuncCall
        Testlib-CallFuncCall KuBoPltHook dl)
target_compile_options(ScalerHook-demoapps-Exceptions PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-FuncCall src/demoapps/TestFuncCall.cpp)
target_link_libraries(ScalerHook-demoapps-FuncCall PUBLIC ScalerHook-HookAutoAsm-C Testlib-FuncCall
        Testlib-CallFuncCall KuBoPltHook dl)
target_compile_options(ScalerHook-demoapps-FuncCall PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-TimingAccuracy src/demoapps/TestTimingAccuracy.cpp)
target_link_libraries(ScalerHook-demoapps-TimingAccuracy PUBLIC Testlib-FuncCall Testlib-CallFuncCall KuBoPltHook dl)
target_compile_options(ScalerHook-demoapps-TimingAccuracy PRIVATE ${TEST_FLAGS} -Werror)
target_link_options(ScalerHook-demoapps-TimingAccuracy PRIVATE "-z" "lazy")

add_executable(ScalerHook-demoapps-HookEverything src/demoapps/TestHookEverything.cpp)
target_include_directories(ScalerHook-demoapps-HookEverything PUBLIC libtest/header)
target_link_libraries(ScalerHook-demoapps-HookEverything PUBLIC ScalerHook-HookAutoAsm-C Testlib-FuncCall
        Testlib-CallFuncCall KuBoPltHook dl)
target_compile_options(ScalerHook-demoapps-HookEverything PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-FuncCallDelay src/demoapps/TestFuncCallDelay.cpp)
target_link_libraries(ScalerHook-demoapps-FuncCallDelay PUBLIC ScalerHook-HookAutoAsm-C Testlib-FuncCall
        Testlib-CallFuncCall dl pthread)
target_compile_options(ScalerHook-demoapps-FuncCallDelay PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-FuncNestedCall src/demoapps/TestNestedFuncCallPthread.cpp)
target_link_libraries(ScalerHook-demoapps-FuncNestedCall PUBLIC ScalerHook-HookAutoAsm-C Testlib-FuncCall
        Testlib-CallFuncCall dl pthread)
target_compile_options(ScalerHook-demoapps-FuncNestedCall PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-Pthread src/demoapps/TestPthread.cpp)
target_include_directories(ScalerHook-demoapps-Pthread PUBLIC libtest/header)
target_link_libraries(ScalerHook-demoapps-Pthread ScalerHook-HookAutoAsm-C Testlib-FuncCall pthread dl)
target_compile_options(ScalerHook-demoapps-Pthread PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-BinaryAccess src/demoapps/TestBinaryAccess.cpp)
target_include_directories(ScalerHook-demoapps-BinaryAccess PUBLIC libtest/header)


add_executable(ScalerHook-proof-Threadlocal src/demoapps/TestThreadLocalAssembly.cpp)
target_include_directories(ScalerHook-proof-Threadlocal PUBLIC libtest/header)
target_link_libraries(ScalerHook-proof-Threadlocal pthread dl)
target_compile_options(ScalerHook-proof-Threadlocal PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-StdPthread src/demoapps/TestStdThread.cpp)
target_include_directories(ScalerHook-demoapps-StdPthread PUBLIC libtest/header)
target_link_libraries(ScalerHook-demoapps-StdPthread ScalerHook-HookAutoAsm-C Testlib-FuncCall Testlib-CallFuncCall pthread dl)
target_compile_options(ScalerHook-demoapps-StdPthread PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-ComplexThreadop src/demoapps/TestComplexThreadOps.cpp)
target_include_directories(ScalerHook-demoapps-ComplexThreadop PUBLIC libtest/header)
target_link_libraries(ScalerHook-demoapps-ComplexThreadop ScalerHook-HookAutoAsm-C Testlib-FuncCall Testlib-CallFuncCall pthread dl)
target_compile_options(ScalerHook-demoapps-ComplexThreadop PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-Semaphore src/demoapps/TestSemaphore.cpp)
target_include_directories(ScalerHook-demoapps-Semaphore PUBLIC)
target_link_libraries(ScalerHook-demoapps-Semaphore ScalerHook-HookAutoAsm-C pthread dl)
target_compile_options(ScalerHook-demoapps-Semaphore PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-Signal src/demoapps/TestSingalHandler.cpp)
target_include_directories(ScalerHook-demoapps-Signal PUBLIC)
target_link_libraries(ScalerHook-demoapps-Signal ScalerHook-HookAutoAsm-C pthread dl)
target_compile_options(ScalerHook-demoapps-Signal PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-InlineFunc src/demoapps/TestInlineFunction.cpp)
target_include_directories(ScalerHook-demoapps-InlineFunc PUBLIC)
target_link_libraries(ScalerHook-demoapps-InlineFunc ScalerHook-HookAutoAsm-C)
target_compile_options(ScalerHook-demoapps-InlineFunc PRIVATE ${TEST_FLAGS} -Werror)

add_executable(ScalerHook-demoapps-Fork src/demoapps/TestFork.cpp)
target_include_directories(ScalerHook-demoapps-Fork PUBLIC)
target_link_libraries(ScalerHook-demoapps-Fork ScalerHook-HookAutoAsm-C Testlib-FuncCall Testlib-CallFuncCall)
target_compile_options(ScalerHook-demoapps-Fork PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-demoapps-SameFuncDiffLib src/demoapps/TestSameFuncDiffLib.cpp)
target_include_directories(ScalerHook-demoapps-SameFuncDiffLib PUBLIC)
target_link_libraries(ScalerHook-demoapps-SameFuncDiffLib ScalerHook-HookAutoAsm-C Testlib-FuncCall)
target_compile_options(ScalerHook-demoapps-SameFuncDiffLib PRIVATE ${TEST_FLAGS} -Werror)


add_executable(ScalerHook-proof-TestTLS src/demoapps/TestTLS.cpp)
target_include_directories(ScalerHook-proof-TestTLS PUBLIC)
target_link_libraries(ScalerHook-proof-TestTLS pthread)
target_compile_options(ScalerHook-proof-TestTLS PRIVATE ${TEST_FLAGS} -Werror)

add_library(libWorkload SHARED src/demoapps/libWorkload.cpp)
target_include_directories(libWorkload PUBLIC src/demoapps/include)

add_executable(ScalerHook-demoapps-SingleThreadedWorkload src/demoapps/TestSimpleFuncCall.cpp)
target_link_libraries(ScalerHook-demoapps-SingleThreadedWorkload PUBLIC libWorkload)

add_executable(ScalerHook-demoapps-ThreadInterleave src/demoapps/TestThreadInterleave.cpp)
target_link_libraries(ScalerHook-demoapps-ThreadInterleave PUBLIC libWorkload pthread)

add_executable(ScalerHook-demoapps-ThreadSimultaneouslyEnd src/demoapps/TestThreadSimultaenouslyEnd.cpp)
target_link_libraries(ScalerHook-demoapps-ThreadSimultaneouslyEnd PUBLIC libWorkload pthread)

add_executable(ScalerHook-demoapps-aget
        src/demoapps/parsecapps/aget/Aget.c
        src/demoapps/parsecapps/aget/Download.c
        src/demoapps/parsecapps/aget/Head.c
        src/demoapps/parsecapps/aget/main.c
        src/demoapps/parsecapps/aget/Misc.c
        src/demoapps/parsecapps/aget/Resume.c
        src/demoapps/parsecapps/aget/Signal.c)
target_include_directories(ScalerHook-demoapps-aget PUBLIC src/demoapps/parsecapps/aget/include)
target_link_libraries(ScalerHook-demoapps-aget ScalerHook-HookAutoAsm-C dl)
target_compile_options(ScalerHook-demoapps-aget PRIVATE ${TEST_FLAGS})


add_executable(ScalerHook-demoapps-dedup
        src/demoapps/parsecapps/dedup/binheap.c
        src/demoapps/parsecapps/dedup/decoder.c
        src/demoapps/parsecapps/dedup/dedup.c
        src/demoapps/parsecapps/dedup/encoder.c
        src/demoapps/parsecapps/dedup/hashtable.c
        src/demoapps/parsecapps/dedup/queue.c
        src/demoapps/parsecapps/dedup/rabin.c
        src/demoapps/parsecapps/dedup/tree.c
        src/demoapps/parsecapps/dedup/util.c)
target_include_directories(ScalerHook-demoapps-dedup PUBLIC src/demoapps/parsecapps/dedup/include)
target_link_directories(ScalerHook-demoapps-dedup PRIVATE src/demoapps/parsecapps/dedup/lib)
target_link_libraries(ScalerHook-demoapps-dedup ScalerHook-HookAutoAsm-C pthread dl
        libcrypto.a
        libssl.a
        libz.a)
target_compile_options(ScalerHook-demoapps-dedup PRIVATE "-DPARALLEL")
target_compile_options(ScalerHook-demoapps-dedup PRIVATE ${TEST_FLAGS})


add_executable(ScalerHook-demoapps-blackscholes src/demoapps/parsecapps/blackscholes/blackscholes-pthread.cpp)
target_link_libraries(ScalerHook-demoapps-blackscholes pthread ScalerHook-HookAutoAsm-C)
target_compile_options(ScalerHook-demoapps-blackscholes PRIVATE ${TEST_FLAGS} "-DENABLE_THREADS" "-DPARALLEL" "-DNCO=2" "-DN=960")


add_executable(ScalerHook-demoapps-swaptions
        src/demoapps/parsecapps/swaptions/CumNormalInv.cpp
        src/demoapps/parsecapps/swaptions/HJM.cpp
        src/demoapps/parsecapps/swaptions/HJM_Securities.cpp
        src/demoapps/parsecapps/swaptions/HJM_SimPath_Forward_Blocking.cpp
        src/demoapps/parsecapps/swaptions/HJM_Swaption_Blocking.cpp
        src/demoapps/parsecapps/swaptions/MaxFunction.cpp
        src/demoapps/parsecapps/swaptions/nr_routines.cpp
        src/demoapps/parsecapps/swaptions/RanUnif.cpp)
target_link_libraries(ScalerHook-demoapps-swaptions pthread ScalerHook-HookAutoAsm-C)
target_compile_options(ScalerHook-demoapps-swaptions PRIVATE ${TEST_FLAGS} "-DENABLE_THREADS" "-fstrict-aliasing" "-fkeep-inline-functions")
target_include_directories(ScalerHook-demoapps-swaptions PRIVATE src/demoapps/parsecapps/swaptions/include)
target_compile_definitions(ScalerHook-demoapps-swaptions PRIVATE ENABLE_THREADS)

find_package(BZip2 REQUIRED)
add_executable(ScalerHook-demoapps-pbzip2
        src/demoapps/parsecapps/pbzip2/BZ2StreamScanner.cpp
        src/demoapps/parsecapps/pbzip2/ErrorContext.cpp
        src/demoapps/parsecapps/pbzip2/pbzip2.cpp)
target_include_directories(ScalerHook-demoapps-pbzip2 PUBLIC src/demoapps/parsecapps/pbzip2/include)
target_link_libraries(ScalerHook-demoapps-pbzip2 ScalerHook-HookAutoAsm-C pthread BZip2::BZip2)
target_compile_options(ScalerHook-demoapps-pbzip2 PRIVATE ${TEST_FLAGS})


#add_executable(ScalerHook-demoapps-sradv1
#        src/parsecapps/srad_v1/main.c
#        )
#target_link_libraries(ScalerHook-demoapps-sradv1  ScalerHook-HookAutoAsm-C pthread m)
#target_link_options(ScalerHook-demoapps-sradv1 PRIVATE "-fopenmp")

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

add_executable(ScalerHook-proof-StackProtector src/proofconcept/TestStackOverflow.cpp)
target_compile_options(ScalerHook-proof-StackProtector PRIVATE "-fstack-protector")

add_executable(ScalerHook-proof-LTraceSimpleMain src/proofconcept/TestLtrace.cpp)
target_link_options(ScalerHook-proof-LTraceSimpleMain PRIVATE "-z" "lazy")

add_executable(ScalerHook-proof-HookEverything src/proofconcept/TestHookEverything.cpp)
target_link_options(ScalerHook-proof-HookEverything PRIVATE "-z" "lazy")
target_link_libraries(ScalerHook-proof-HookEverything Testlib-FuncCall Testlib-CallFuncCall)
target_compile_options(ScalerHook-proof-HookEverything PRIVATE ${TEST_FLAGS} "-z" "lazy")

add_executable(ScalerHook-proof-PthreadParam src/proofconcept/testPthreadParameter.cpp)
target_link_options(ScalerHook-proof-PthreadParam PRIVATE "-z" "now" "-O0") #Avoid plt lookup code.
target_link_libraries(ScalerHook-proof-PthreadParam pthread)

#add_executable(ScalerHook-proof-SwBreakpoint src/proofconcept/testSwBreakpoint.cpp)

add_executable(ScalerHook-proof-ManualBreakpoint src/proofconcept/testManualBreakpoint.cpp)

add_executable(ScalerHook-proof-ObjCopy src/proofconcept/TestObjCopy.cpp)

add_executable(ScalerHook-proof-PtraceReadProcMem src/proofconcept/testPtraceReadProcMemory.cpp)

add_executable(ScalerHook-proof-testMultithreadPrint src/proofconcept/testMultithreadPrint.cpp)
target_link_libraries(ScalerHook-proof-testMultithreadPrint pthread)
target_link_options(ScalerHook-proof-testMultithreadPrint PRIVATE "-z" "lazy" "-O0")

add_executable(ScalerHook-proof-SaveDataUponExit src/proofconcept/testSaveDataUponExit.cpp)
target_link_libraries(ScalerHook-proof-SaveDataUponExit pthread)

add_executable(ScalerHook-proof-threadlocalasmarray src/proofconcept/TestAccessThreadLocalArrayInAsm.cpp)
target_link_libraries(ScalerHook-proof-threadlocalasmarray pthread)


find_package(Python3 COMPONENTS Development.Module REQUIRED)

add_executable(ScalerHook-proof-PythonModuleInterception src/proofconcept/ScalerPythonModule/TestPythonModuleInterception.cpp)
target_link_libraries(ScalerHook-proof-PythonModuleInterception PUBLIC pthread Python3::Module)

add_executable(GetUserSysRealTime src/proofconcept/getUserTime.cpp src/demoapps/libWorkload.cpp src/demoapps/include/libWorkload.h)
target_link_libraries(GetUserSysRealTime)


#add_executable(ScalerHook-demoapps src/proofconcept/testprog.cpp)
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

#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../meta/addressbook.proto)

