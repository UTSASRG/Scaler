
add_library(unwind-wat IMPORTED STATIC)
set_target_properties(unwind-wat PROPERTIES IMPORTED_LOCATION lib/libunwind-x86_64.la)
set_target_properties(unwind-wat PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
