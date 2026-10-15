add_library(lir_headers INTERFACE)
target_include_directories(lir_headers INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(lir_exec main.cpp)
target_link_libraries(lir_exec PRIVATE lir_headers lang_lib ir)