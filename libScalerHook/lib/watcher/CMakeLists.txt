
add_subdirectory(lib/libipt)
add_subdirectory(lib/heaplayers)
add_subdirectory(lib/libunwind)
add_subdirectory(lib/xed2)

#src/libwatcher.cpp
#src/instbuffer.cpp
#src/internalsyncs.cpp
#src/leakcheck.cpp
#src/memtrack.cpp
#src/perthread_decoder.cpp
#src/pt_manager.cpp
#src/quarantine.cpp
#src/rca_disas.cpp
#src/rca_tool.cpp
#src/real.cpp
#src/recordentries.cpp
#src/selfmap.cpp
#src/software_breakpoint.cpp
#src/vm_emulator.cpp
#src/watchpoint.cpp
#src/xmemory.cpp
#src/xrun.cpp
#src/xthread.cpp
add_library(watcher SHARED
            src/software_breakpoint.cpp
            )
target_link_libraries(watcher PUBLIC pthread ipt-wat unwind-wat xed2-wat heaplayer-wac)
target_include_directories(watcher PUBLIC include)
target_compile_definitions(watcher PUBLIC SINGLELL
        ENABLE_WATCHPOINT_DETECTOR
        NINTERACTIVE_DEBUG
        NDUMP_LOG_TODISK
        TESTSEGV
        SSE_SUPPORT
        DL_SUPPORT
        SUPPROT_MULTILEVEL
        HAVE_SOFTWARE_BREAKPOINT
        GET_REPLAY_RUNTIME
        NGET_INST_COUNT)
