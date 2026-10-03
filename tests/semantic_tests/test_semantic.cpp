#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

#include "lang_lexer.hpp"
#include "parser.tab.hpp"
#include "ast.hpp"
#include "semantic.hpp"

extern std::shared_ptr<ast::program> program;
extern int syntax_errors;
extern int lexical_errors;

class semantic_test : public ::testing::Test {
protected:
    std::stringstream captured_cerr_;
    std::streambuf* old_cerr_ = nullptr;

    void SetUp() override {
        old_cerr_ = std::cerr.rdbuf();
        std::cerr.rdbuf(captured_cerr_.rdbuf());


        program = std::make_shared<ast::program>();
        syntax_errors = 0;
        lexical_errors = 0;
    }

    std::vector<sema::error> run(const std::string& filename) {
        std::string path = std::string(PROG_DIR) + "/" + filename;
        std::ifstream file(path);
        EXPECT_TRUE(file) << "Cannot open " << path;
        if (!file) return {};

        yy::lang_lexer lexer(file);
        yy::Parser parser(lexer);
        parser.parse();

        EXPECT_EQ(lexical_errors, 0) << "lexical errors in " << filename;
        EXPECT_EQ(syntax_errors, 0) << "syntax errors in " << filename;

        sema::analyzer a;
        a.analyze(program);
        return a.get_errors();
    }
    void TearDown() override {
        program.reset();
    }

    static bool has(const std::vector<sema::error>& errors,
                    const std::string& substr) {
        for (const auto& e : errors) {
            if (e.msg_.find(substr) != std::string::npos) return true;
        }
        return false;
    }

};


TEST_F(semantic_test, valid) {
    auto errors = run("sem_valid.lang");
    EXPECT_TRUE(errors.empty());
}

// ОБЛАСТИ ВИДИМОСТИ

TEST_F(semantic_test, undeclared_variable) {
    auto errors = run("sem_undeclared_var.lang");
    EXPECT_TRUE(has(errors, "not declared"));
}

TEST_F(semantic_test, undeclared_function) {
    auto errors = run("sem_undeclared_func.lang");
    EXPECT_TRUE(has(errors, "not declared"));
}

TEST_F(semantic_test, duplicate_variable) {
    auto errors = run("sem_duplicate_var.lang");
    EXPECT_TRUE(has(errors, "already declared"));
}

TEST_F(semantic_test, duplicate_param) {
    auto errors = run("sem_duplicate_param.lang");
    EXPECT_TRUE(has(errors, "already declared"));
}

TEST_F(semantic_test, duplicate_function) {
    auto errors = run("sem_duplicate_func.lang");
    EXPECT_TRUE(has(errors, "already defined"));
}

TEST_F(semantic_test, no_main) {
    auto errors = run("sem_no_main.lang");
    EXPECT_TRUE(has(errors, "main"));
}


// ТИПЫ

TEST_F(semantic_test, assign_type) {
    auto errors = run("sem_assign_type.lang");
    EXPECT_TRUE(has(errors, "assign"));
}

TEST_F(semantic_test, arith_non_numeric) {
    auto errors = run("sem_arith_non_numeric.lang");
    EXPECT_TRUE(has(errors, "numeric"));
}

TEST_F(semantic_test, bitwise_non_numeric) {
    auto errors = run("sem_bitwise_non_numeric.lang");
    EXPECT_TRUE(has(errors, "numeric"));
}

TEST_F(semantic_test, logical_non_bool) {
    auto errors = run("sem_logical_non_bool.lang");
    EXPECT_TRUE(has(errors, "bool"));
}

TEST_F(semantic_test, compare_different) {
    auto errors = run("sem_compare_different.lang");
    EXPECT_TRUE(has(errors, "different"));
}

TEST_F(semantic_test, if_non_bool) {
    auto errors = run("sem_if_non_bool.lang");
    EXPECT_TRUE(has(errors, "bool"));
}

TEST_F(semantic_test, while_non_bool) {
    auto errors = run("sem_while_non_bool.lang");
    EXPECT_TRUE(has(errors, "bool"));
}

TEST_F(semantic_test, unary_neg_non_numeric) {
    auto errors = run("sem_unary_neg_non_numeric.lang");
    EXPECT_TRUE(has(errors, "numeric"));
}

TEST_F(semantic_test, unary_not_non_bool) {
    auto errors = run("sem_unary_not_non_bool.lang");
    EXPECT_TRUE(has(errors, "bool"));
}

TEST_F(semantic_test, call_wrong_args) {
    auto errors = run("sem_call_wrong_args.lang");
    EXPECT_TRUE(has(errors, "expects"));
}

TEST_F(semantic_test, index_non_array) {
    auto errors = run("sem_index_non_array.lang");
    EXPECT_TRUE(has(errors, "non-array"));
}

TEST_F(semantic_test, index_non_numeric) {
    auto errors = run("sem_index_non_numeric.lang");
    EXPECT_TRUE(has(errors, "index"));
}

TEST_F(semantic_test, return_mismatch) {
    auto errors = run("sem_return_mismatch.lang");
    EXPECT_TRUE(has(errors, "return"));
}