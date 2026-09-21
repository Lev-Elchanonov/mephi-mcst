#include <gtest/gtest.h>
#include <algorithm>
#include "graph.hpp"

class tree_test : public ::testing::Test {
protected:
    tree::tree_t<std::string, int> tree_;

    void SetUp() override {
        auto A = tree_.add_node("A");
        auto B = tree_.add_node("B");
        auto C = tree_.add_node("C");
        auto D = tree_.add_node("D");
        auto E = tree_.add_node("E");
        auto F = tree_.add_node("F");

        tree_.add_edge(A, B, 1);
        tree_.add_edge(A, C, 2);
        tree_.add_edge(B, D, 3);
        tree_.add_edge(B, E, 4);
        tree_.add_edge(C, F, 5);
    }
};

TEST_F(tree_test, Tree_has_root) {
    EXPECT_FALSE(tree_.get_root() == tree_.nend());
}

TEST_F(tree_test, Tree_has_not_root) {
    auto A = tree_.find_node("A");
    tree_.delete_node(A);
    EXPECT_EQ(tree_.get_root(), tree_.nend());
}

TEST_F(tree_test, Tree_root_is_A) {
    auto root = tree_.get_root();
    ASSERT_NE(root, tree_.nend());
    EXPECT_EQ(root->get_data(), "A");
}

TEST_F(tree_test, Tree_root_has_no_incoming_edges) {
    auto root = tree_.get_root();
    ASSERT_NE(root, tree_.nend());
    EXPECT_TRUE(root->incoming_edges().empty());
}

TEST_F(tree_test, add_inicominng_edge_to_root) {
    tree_.add_node("Z");
    auto Z = tree_.find_node("Z");
    auto A = tree_.find_node("A");
    tree_.add_edge(Z, A, 3);
    EXPECT_DEATH(tree_.check_validation(), "Error: root can not have incoming edges");
}


TEST_F(tree_test, Tree_has_no_loops) {
    EXPECT_NO_FATAL_FAILURE(tree_.check_validation());
}

TEST_F(tree_test, Tree_add_back_edge_creates_loop) {
    auto D = tree_.find_node("D");
    auto A = tree_.find_node("A");
    ASSERT_NE(D, tree_.nend());
    ASSERT_NE(A, tree_.nend());

    // тут backedge
    tree_.add_edge(D, A, 99);
    EXPECT_DEATH(tree_.check_validation(), "Error: tree can not have loops" );
}

TEST_F(tree_test, delete_root) {
    auto root = tree_.get_root();
    tree_.delete_node(root);
    EXPECT_TRUE(tree_.get_nodes().empty());

}