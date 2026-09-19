add_subdirectory(i_command)
add_subdirectory(add_edge_cmd)
add_subdirectory(add_node_cmd)
add_subdirectory(delete_edge_cmd)
add_subdirectory(delete_node_cmd)
add_subdirectory(visualize_cmd)
add_subdirectory(clear_cmd)
add_subdirectory(dfs_cmd)
add_subdirectory(set_dest_cmd)
add_subdirectory(set_source_cmd)


add_library(command_deps INTERFACE)
target_link_libraries(command_deps INTERFACE
        i_command
        add_edge_cmd
        add_vertex_cmd
        delete_edge_cmd
        delete_node_cmd
        visualize_cmd
        clear_cmd
        set_source_cmd
        set_dest_cmd
        dfs_cmd
)