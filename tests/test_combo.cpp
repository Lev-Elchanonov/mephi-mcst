#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include "graph.hpp"


class graph_combination_test : public ::testing::Test {
protected:
    graph::orgraph_t<std::string, int> graph_;
};


TEST_F(graph_combination_test, add_edge_set_source_set_dest_delete_edge) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    auto C = graph_.add_node("C");
    auto D = graph_.add_node("D");

    auto e = graph_.add_edge(A, B, 1);
    ASSERT_NE(e, graph_.eend());

    graph_.set_source(e, C);
    EXPECT_EQ(e->get_source(), C);
    EXPECT_EQ(e->get_dest(), B);

    graph_.set_dest(e, D);
    EXPECT_EQ(e->get_source(), C);
    EXPECT_EQ(e->get_dest(), D);

    graph_.delete_edge(e);
    EXPECT_EQ(graph_.get_edges().size(), 0);

    EXPECT_EQ(graph_.get_nodes().size(), 4);

    EXPECT_EQ(graph_.find_edge("C", "D"), graph_.eend());
}

TEST_F(graph_combination_test, delete_node_then_add) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    graph_.add_edge(A, B, 1);

    graph_.delete_node(A);

    EXPECT_EQ(graph_.find_node("A"), graph_.nend());
    EXPECT_EQ(graph_.get_nodes().size(), 1);
    EXPECT_EQ(graph_.get_edges().size(), 0);

    auto A2 = graph_.add_node("A");
    EXPECT_NE(A2, graph_.nend());
    EXPECT_EQ(A2->get_data(), "A");
    EXPECT_EQ(A2->incoming_edges().size(), 0);
    EXPECT_EQ(A2->outgoing_edges().size(), 0);
}

TEST_F(graph_combination_test, delete_source_remove_edgee) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");

    graph_.delete_node(A);

    EXPECT_EQ(graph_.get_edges().size(), 0);
    EXPECT_EQ(B->incoming_edges().size(), 0);
    EXPECT_EQ(graph_.find_edge("A", "B"), graph_.eend());
}


TEST_F(graph_combination_test, delete_dest_remove_edge) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");

    graph_.delete_node(B);

    EXPECT_EQ(graph_.get_edges().size(), 0);
    EXPECT_EQ(A->outgoing_edges().size(), 0);
    EXPECT_EQ(graph_.find_edge("A", "B"), graph_.eend());
}


TEST_F(graph_combination_test, multi_edges_delete_one) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    auto C = graph_.add_node("C");

    auto e1 = graph_.add_edge(A, B, 1);
    auto e2 = graph_.add_edge(B, C, 2);
    auto e3 = graph_.add_edge(A, C, 3);

    ASSERT_EQ(graph_.get_edges().size(), 3);

    graph_.delete_edge(e2);
    EXPECT_EQ(graph_.get_edges().size(), 2);

    EXPECT_NE(graph_.find_edge("A", "B"), graph_.eend());
    EXPECT_EQ(graph_.find_edge("B", "C"), graph_.eend());
    EXPECT_NE(graph_.find_edge("A", "C"), graph_.eend());

    EXPECT_EQ(e1->get_source(), A);
    EXPECT_EQ(e1->get_dest(), B);
    EXPECT_EQ(e3->get_source(), A);
    EXPECT_EQ(e3->get_dest(), C);
}


TEST_F(graph_combination_test, set_source_twice) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    auto C = graph_.add_node("C");
    auto D = graph_.add_node("D");

    auto e = graph_.add_edge(A, B, 1);

    graph_.set_source(e, C);
    EXPECT_EQ(e->get_source(), C);
    EXPECT_EQ(A->outgoing_edges().size(), 0);
    EXPECT_EQ(C->outgoing_edges().size(), 1);

    graph_.set_source(e, D);
    EXPECT_EQ(e->get_source(), D);
    EXPECT_EQ(C->outgoing_edges().size(), 0);
    EXPECT_EQ(D->outgoing_edges().size(), 1);
}


TEST_F(graph_combination_test, set_dest_twice) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    auto C = graph_.add_node("C");
    auto D = graph_.add_node("D");

    auto e = graph_.add_edge(A, B, 1);

    graph_.set_dest(e, C);
    EXPECT_EQ(e->get_dest(), C);
    EXPECT_EQ(B->incoming_edges().size(), 0);
    EXPECT_EQ(C->incoming_edges().size(), 1);

    graph_.set_dest(e, D);
    EXPECT_EQ(e->get_dest(), D);
    EXPECT_EQ(C->incoming_edges().size(), 0);
    EXPECT_EQ(D->incoming_edges().size(), 1);
}


TEST_F(graph_combination_test, self_loop) {
    auto A = graph_.add_node("A");

    auto e = graph_.add_edge(A, A, 1);
    ASSERT_NE(e, graph_.eend());

    EXPECT_EQ(e->get_source(), A);
    EXPECT_EQ(e->get_dest(), A);
    EXPECT_EQ(A->outgoing_edges().size(), 1);
    EXPECT_EQ(A->incoming_edges().size(), 1);
}


TEST_F(graph_combination_test, clear_after_many_ops) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");
    auto C = graph_.add_node("C");
    graph_.add_edge(A, B, 1);
    graph_.add_edge(B, C, 2);
    graph_.add_edge(A, C, 3);

    graph_.set_dest(graph_.find_edge("A", "B"), C);

    graph_.clear();

    EXPECT_EQ(graph_.get_nodes().size(), 0);
    EXPECT_EQ(graph_.get_edges().size(), 0);
    EXPECT_EQ(graph_.find_node("A"), graph_.nend());
    EXPECT_EQ(graph_.find_edge("A", "B"), graph_.eend());
}


TEST_F(graph_combination_test, add_node_duplicate) {
    auto A = graph_.add_node("A");
    auto A2 = graph_.add_node("A");

    EXPECT_EQ(A2, graph_.nend());

    EXPECT_EQ(graph_.get_nodes().size(), 1);

    EXPECT_EQ(graph_.find_node("A"), A);
}


TEST_F(graph_combination_test, add_edge_duplicate) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");

    auto e1 = graph_.add_edge(A, B, 1);
    auto e2 = graph_.add_edge(A, B, 2);

    EXPECT_NE(e1, graph_.eend());
    EXPECT_EQ(e2, graph_.eend());

    EXPECT_EQ(graph_.get_edges().size(), 1);
}




TEST_F(graph_combination_test, re_add_edge_after_delete) {
    auto A = graph_.add_node("A");
    auto B = graph_.add_node("B");

    auto e1 = graph_.add_edge(A, B, 1);
    graph_.delete_edge(e1);

    EXPECT_EQ(graph_.get_edges().size(), 0);

    auto e2 = graph_.add_edge(A, B, 2);
    ASSERT_NE(e2, graph_.eend());
    EXPECT_EQ(e2->get_data(), 2);
    EXPECT_EQ(graph_.get_edges().size(), 1);
}

