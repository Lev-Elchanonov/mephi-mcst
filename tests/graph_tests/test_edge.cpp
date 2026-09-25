#include <gtest/gtest.h>

#include "graph.hpp"

class edge_test : public ::testing::Test {
protected:
    graph::orgraph_t<std::string, int>graph_;
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


TEST_F(edge_test, Add_new_edge) {
    auto prev_size = graph_.get_edges().size();

    auto A = graph_.find_node("A");
    auto E = graph_.find_node("E");

    auto edge_iter = graph_.add_edge(A, E, 10);

    EXPECT_FALSE(edge_iter == graph_.eend());
    EXPECT_EQ(graph_.get_edges().size(), prev_size + 1);

    EXPECT_EQ(edge_iter->get_source(), A);
    EXPECT_EQ(edge_iter->get_dest(), E);
    EXPECT_EQ(edge_iter->get_data(), 10);

    auto& a_out = A->outgoing_edges();
    EXPECT_NE(std::find(a_out.begin(), a_out.end(), edge_iter), a_out.end());

    auto& e_in = E->incoming_edges();
    EXPECT_NE(std::find(e_in.begin(), e_in.end(), edge_iter), e_in.end());
}

TEST_F(edge_test, Add_exist_edge) {
    auto prev_size = graph_.get_edges().size();

    auto A = graph_.find_node("A");
    auto B = graph_.find_node("B");

    auto edge_iter = graph_.add_edge(A, B, 999);

    EXPECT_EQ(graph_.get_edges().size(), prev_size);


    EXPECT_EQ(edge_iter, graph_.eend());
    auto existing = graph_.find_edge("A", "B");
    EXPECT_NE(existing, graph_.eend());
    EXPECT_EQ(existing->get_data(), 5);
}

TEST_F(edge_test, Add_edge_with_invalid_node) {
    auto prev_size = graph_.get_edges().size();

    auto A = graph_.find_node("A");
    auto unknown = graph_.nend();

    auto edge_iter = graph_.add_edge(A, unknown, 10);

    EXPECT_EQ(edge_iter, graph_.eend());
    EXPECT_EQ(graph_.get_edges().size(), prev_size);
}


TEST_F(edge_test, Delete_exist_edge) {
    auto prev_size = graph_.get_edges().size();

    auto A = graph_.find_node("A");
    auto B = graph_.find_node("B");

    auto edge_iter = graph_.find_edge("A", "B");
    ASSERT_NE(edge_iter, graph_.eend());

    graph_.delete_edge(edge_iter);

    EXPECT_EQ(graph_.get_edges().size(), prev_size - 1);

    EXPECT_EQ(graph_.find_edge("A", "B"), graph_.eend());

    auto& a_out = A->outgoing_edges();
    EXPECT_EQ(std::find(a_out.begin(), a_out.end(), edge_iter), a_out.end());

    auto& b_in = B->incoming_edges();
    EXPECT_EQ(std::find(b_in.begin(), b_in.end(), edge_iter), b_in.end());
}

TEST_F(edge_test, Delete_unknown_edge) {
    auto prev_size = graph_.get_edges().size();

    graph_.delete_edge(graph_.eend());

    EXPECT_EQ(graph_.get_edges().size(), prev_size);
}


TEST_F(edge_test, Set_source_existing_edge) {
    auto A = graph_.find_node("A");
    auto B = graph_.find_node("B");
    auto C = graph_.find_node("C");

    auto edge_iter = graph_.find_edge("A", "B");
    ASSERT_NE(edge_iter, graph_.eend());

    // меняем source на C: станет C->B
    graph_.set_source(edge_iter, C);

    EXPECT_EQ(edge_iter->get_source(), C);
    EXPECT_EQ(edge_iter->get_dest(), B);

    auto& a_out = A->outgoing_edges();
    EXPECT_EQ(std::find(a_out.begin(), a_out.end(), edge_iter), a_out.end());

    auto& c_out = C->outgoing_edges();
    EXPECT_NE(std::find(c_out.begin(), c_out.end(), edge_iter), c_out.end());

    auto& b_in = B->incoming_edges();
    EXPECT_NE(std::find(b_in.begin(), b_in.end(), edge_iter), b_in.end());
}

TEST_F(edge_test, Set_dest_existing_edge) {
    auto A = graph_.find_node("A");
    auto B = graph_.find_node("B");
    auto D = graph_.find_node("D");

    auto edge_iter = graph_.find_edge("A", "B");
    ASSERT_NE(edge_iter, graph_.eend());

    // меняем dest на D,  станет A->D
    graph_.set_dest(edge_iter, D);

    EXPECT_EQ(edge_iter->get_source(), A);
    EXPECT_EQ(edge_iter->get_dest(), D);

    auto& b_in = B->incoming_edges();
    EXPECT_EQ(std::find(b_in.begin(), b_in.end(), edge_iter), b_in.end());


    auto& d_in = D->incoming_edges();
    EXPECT_NE(std::find(d_in.begin(), d_in.end(), edge_iter), d_in.end());

    auto& a_out = A->outgoing_edges();
    EXPECT_NE(std::find(a_out.begin(), a_out.end(), edge_iter), a_out.end());
}

TEST_F(edge_test, Set_source_unknown_edge) {
    auto C = graph_.find_node("C");

    graph_.set_source(graph_.eend(), C);

    EXPECT_NE(graph_.find_edge("A", "B"), graph_.eend());


    EXPECT_EQ(graph_.find_edge("C", "A"), graph_.eend());
}


TEST_F(edge_test, Set_dest_unknown_edge) {
    auto D = graph_.find_node("D");

    graph_.set_dest(graph_.eend(), D);

    EXPECT_NE(graph_.find_edge("A", "B"), graph_.eend());

    EXPECT_EQ(graph_.find_edge("A", "D"), graph_.eend());
}


TEST_F(edge_test, Set_source_invalid_node) {
    auto A = graph_.find_node("A");

    auto edge_iter = graph_.find_edge("A", "B");
    ASSERT_NE(edge_iter, graph_.eend());

    graph_.set_source(edge_iter, graph_.nend());

    EXPECT_EQ(edge_iter->get_source(), A);
}

TEST_F(edge_test, Set_dest_invalid_node) {
    auto B = graph_.find_node("B");

    auto edge_iter = graph_.find_edge("A", "B");
    ASSERT_NE(edge_iter, graph_.eend());

    graph_.set_dest(edge_iter, graph_.nend());

    EXPECT_EQ(edge_iter->get_dest(), B);
}