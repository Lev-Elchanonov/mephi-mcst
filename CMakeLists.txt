cmake_minimum_required(VERSION 3.20)
project(graphs LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

option(SAN "Enable ASan and UBSan" OFF)
option(DEBUG "Define DEBUG macro" OFF)
option(VIEW "Build view library (graph visualization)" OFF)
option(AST "Build AST" OFF)

if(DEBUG)
    add_compile_definitions(DEBUG)
    message(STATUS "DEBUG macro defined")
endif()

if(SAN)
    message(STATUS "Enabling ASan and UBSan")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
else()
    message(STATUS "Sanitizers disabled")
endif()

add_subdirectory(tools)

if(VIEW)
    message(STATUS "VIEW enabled: building view library")
    add_subdirectory(view_lib)

    add_executable(graph view_lib/main.cpp)
    target_link_libraries(graph PRIVATE
            view
            controller
            command_deps
            tools
    )
    target_compile_definitions(graph PRIVATE VIEW_ENABLED)
else()
    message(STATUS "VIEW disabled: building minimal graph")
    add_executable(graph main.cpp)
    target_link_libraries(graph PRIVATE tools)
endif()

if(AST)
    message(STATUS "Building frontend (AST)")
    add_subdirectory(frontend)
else()
    message(STATUS "Frontend disabled (AST=OFF)")
endif()
