
add_library(xed2-wat IMPORTED SHARED GLOBAL)
set_target_properties(xed2-wat PROPERTIES IMPORTED_LOCATION lib/libxed-ild.so)
set_target_properties(xed2-wat PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
