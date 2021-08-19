
add_library(ipt-wat IMPORTED SHARED GLOBAL)
set_target_properties(ipt-wat PROPERTIES IMPORTED_LOCATION lib/libipt-wat.so.2.0.0)
set_target_properties(ipt-wat PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include)
