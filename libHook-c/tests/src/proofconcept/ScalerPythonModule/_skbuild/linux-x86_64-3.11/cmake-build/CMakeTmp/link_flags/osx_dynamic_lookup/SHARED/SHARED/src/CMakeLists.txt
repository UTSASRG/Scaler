
      cmake_minimum_required(VERSION 3.26.3)
      project(link_flags C)

      include_directories(/home/steven/Projects/Scaler/libHook-c/tests/src/proofconcept/ScalerPythonModule/_skbuild/linux-x86_64-3.11/cmake-build/CMakeTmp/link_flags/osx_dynamic_lookup/SHARED/SHARED/src)

      add_library(number SHARED number.c)
      add_library(counter SHARED counter.c)
    
        target_link_libraries(counter  number)
      
      add_executable(main main.c)
    
        target_link_libraries(main  number)
      
        target_link_libraries(main  "dl")
      