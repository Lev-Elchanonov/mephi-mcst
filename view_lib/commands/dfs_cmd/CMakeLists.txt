add_library(dfs_cmd INTERFACE)
target_include_directories(dfs_cmd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dfs_cmd INTERFACE i_command)