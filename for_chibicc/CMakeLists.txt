add_library(chibicc_headers INTERFACE)
target_include_directories(chibicc_headers INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vm
)

add_executable(cgen main_cgen.cpp)
target_link_libraries(cgen PRIVATE
        chibicc_headers
        lir_headers
        lang_lib
        ir
)