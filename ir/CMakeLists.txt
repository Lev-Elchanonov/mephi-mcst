add_library(ir INTERFACE)
target_include_directories(ir INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/frontend/src
)

add_executable(ir_test main.cpp)
target_link_libraries(ir_test PRIVATE ir lang_lib)