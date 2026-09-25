add_library(graph_tests_lib OBJECT
        test_node.cpp
        test_edge.cpp
        test_tree.cpp
        test_combo.cpp
)


target_link_libraries(graph_tests_lib PUBLIC
        GTest::gtest
        tools
)