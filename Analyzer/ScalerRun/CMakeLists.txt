add_subdirectory(lib/yamlcpp_0_7_0)
add_executable(ScalerRun src/injector.cpp)
target_link_libraries(ScalerRun yaml-cpp::yaml-cpp ScalerHook-Config)




