# Compile LibPltHook
project(ScalerHook)

# Set global (Relative this folder until reset) compiler options
set(CMAKE_C_STANDARD 11)

# Scaler hook library itself. Users need to manually install scalerhook through library functions.
add_library(libScalerHook-HookManual SHARED
        src/ExtFuncCallHook_Linux.cpp
        src/ExtFuncCallHookAsm.cpp
        src/ProcInfoParser.cpp
        src/ElfParser.cpp
        src/MemTool.cpp
        src/FileTool.cpp
        src/install.cpp
        src/Timer.cpp
        src/Config.cpp
        src/StringTool.cpp)
target_include_directories(libScalerHook-HookManual PUBLIC src/include lib/inireader)
target_compile_options(libScalerHook-HookManual PRIVATE "--verbose;-znow;-fPIE;-fPIC;")
target_link_libraries(libScalerHook-HookManual PRIVATE pthread dl)

#add_library(libScalerHook-HookManualPtrace SHARED
#        src/ExtFuncCallHookPtrace.cpp
#        src/ProcInfoParser.cpp
#        src/ElfParser.cpp
#        src/MemTool.cpp
#        src/FileTool.cpp
#        src/install.cpp
#        src/StringTool.cpp)
#target_include_directories(libScalerHook-HookManualPtrace PUBLIC src/include)
#target_compile_options(libScalerHook-HookManualPtrace PRIVATE "--verbose;-znow;-fPIE;-fPIC;")
#target_link_libraries(libScalerHook-HookManualPtrace PRIVATE pthread dl)


#ScalerHook with auto installation enabled. Scaler hook will run automatically before main.
add_library(libScalerHook-HookAuto SHARED src/autoinstall.cpp)
target_compile_options(libScalerHook-HookAuto PRIVATE "--verbose" "-znow" "-fPIC")
target_link_libraries(libScalerHook-HookAuto PRIVATE libScalerHook-HookManual pthread dl)

#Add tests
add_subdirectory(tests)