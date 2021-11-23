
add_library(unwind-wat IMPORTED SHARED GLOBAL)
set_target_properties(unwind-wat PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/libunwind-wat-x86_64.so.8.0.1)
set_target_properties(unwind-wat PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
