#include <gtest/gtest.h>
#include <algorithm>
#include "graph.hpp"

class node_test : public ::testing::Test {
protected:
    graph::orgraph_t<std::string, int> graph_;
    void SetUp() override {
        graph_.add_node("A");
        graph_.add_node("B");
        graph_.add_node("C");
        graph_.add_node("D");
        graph_.add_node("E");


        auto A = graph_.find_node("A");
        auto B = graph_.find_node("B");
        auto C = graph_.find_node("C");
        auto D = graph_.find_node("D");
        auto E = graph_.find_node("E");

        graph_.add_edge( A, B, 5);
        graph_.add_edge( A, C, 3);
        graph_.add_edge( B, C, 2);
        graph_.add_edge( B, D, 4);
        graph_.add_edge( C, D, 1);
        graph_.add_edge( C, E, 6);
        graph_.add_edge( D, E, 3);
    }
};


TEST_F(node_test, Add_new_node) {
    auto iter = graph_.add_node("PP");
    EXPECT_FALSE(iter == graph_.nend());
    EXPECT_FALSE(graph_.find_node("PP") == graph_.nend());
    EXPECT_EQ(graph_.find_node("PP")->get_data(), "PP");
    EXPECT_EQ(graph_.find_node("PP")->outgoing_edges().size(), 0);
}

TEST_F(node_test, Add_exist_node) {
    auto iter_before = graph_.find_node("A");
    auto o_edges_b = iter_before->outgoing_edges();
    auto i_edges_b = iter_before->incoming_edges();

    auto iter_after = graph_.add_node("A");
    EXPECT_EQ(graph_.nend(), iter_after);
}



TEST_F(node_test, Delete_exist_node) {
    auto prev_size = graph_.get_nodes().size();
    auto node_iter = graph_.find_node("A");
    graph_.delete_node(node_iter);
    auto current_size = graph_.get_nodes().size();
    EXPECT_EQ(prev_size - 1, current_size);

    auto all_nodes = graph_.get_nodes();
    EXPECT_EQ(graph_.find_node("A"), graph_.nend());
}

TEST_F(node_test, Delete_unknown_node) {
    auto prev_size = graph_.get_nodes().size();

    auto unknown = graph_.nend();
    graph_.delete_node(unknown);
    auto cur_size = graph_.get_nodes().size();
    EXPECT_EQ(prev_size, cur_size);
}
