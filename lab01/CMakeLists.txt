cmake_minimum_required(VERSION 3.20)
project(graphs LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    message(STATUS "DEBUG macro defined")
endif()

add_compile_options(-Wall -Wextra -Wpedantic)


if(TYPE STREQUAL "Debug")
    message(STATUS "Enabling ASan and UBSan for debug build")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
else()
    message(STATUS "Sanitizers disabled for non-debug build")
endif()

add_subdirectory(tools)
add_subdirectory(commands)
add_subdirectory(controller)
add_subdirectory(view)


add_executable(graph main.cpp)
target_link_libraries(graph PRIVATE
        view
        controller
        tools
        command_deps
)


find_package(GTest QUIET)
if(GTest_FOUND)
    enable_testing()
    add_executable(tests
            tests/test_main.cpp
            tests/test_node.cpp
            tests/test_edge.cpp
            tests/test_tree.cpp
            tests/test_combo.cpp
    )
    target_link_libraries(tests PRIVATE
            GTest::gtest
            GTest::gtest_main
            controller
            tools
            command_deps
    )
    add_test(NAME tests COMMAND tests)
else()
    message(WARNING "Google Test not found. Tests will not be built.")
endif()