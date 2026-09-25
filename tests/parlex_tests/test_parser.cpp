#include <gtest/gtest.h>
#include <fstream>
#include <string>

#include "lang_lexer.hpp"
#include "parser.tab.hpp"
#include "ast.hpp"

extern std::shared_ptr<ast::program> program;
extern int syntax_errors;
extern int lexical_errors;

class parser_test : public ::testing::Test {
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

    void run_valid(const std::string& filename) {
        std::string path = std::string(VALID_DIR) + "/" + filename;
        std::ifstream file(path);
        ASSERT_TRUE(file) << "Cannot open " << path;

        yy::lang_lexer lexer(file);
        yy::Parser parser(lexer);
        parser.parse();
    }
    void run_synerr(const std::string& filename) {
        std::string path = std::string(SYNERR_DIR) + "/" + filename;
        std::ifstream file(path);
        ASSERT_TRUE(file) << "Cannot open " << path;

        yy::lang_lexer lexer(file);
        yy::Parser parser(lexer);
        parser.parse();
    }
    void run_lexerr(const std::string& filename) {
        std::string path = std::string(LEXERR_DIR) + "/" + filename;
        std::ifstream file(path);
        ASSERT_TRUE(file) << "Cannot open " << path;

        yy::lang_lexer lexer(file);
        yy::Parser parser(lexer);
        parser.parse();
    }

};

TEST_F(parser_test, valid_arrays) {
    run_valid("valid_arrays.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_example) {
    run_valid("valid_example.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_full) {
    run_valid("valid_full.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_ifelse) {
    run_valid("valid_ifelse.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_minim) {
    run_valid("valid_minim.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_nested) {
    run_valid("valid_nested.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_operations) {
    run_valid("valid_operations.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, valid_pointers) {
    run_valid("valid_pointers.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 0);
}



TEST_F(parser_test, synerr_bad_call) {
    run_synerr("synerr_bad_call.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 1);
}

TEST_F(parser_test, synerr_bad_if) {
    run_synerr("synerr_bad_if.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_NE(syntax_errors, 0);
}

TEST_F(parser_test, synerr_bad_prog) {
    run_synerr("synerr_bad_prog.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 1);
}

TEST_F(parser_test, synerr_bad_while) {
    run_synerr("synerr_bad_while.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 2);
}

TEST_F(parser_test, synerr_missing_semi) {
    run_synerr("synerr_missing_semi.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 1);
}

TEST_F(parser_test, synerr_uncl_brase) {
    run_synerr("synerr_uncl_brase.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 1);
}

TEST_F(parser_test, synerr_not_number) {
    run_synerr("synerr_not_number.lang");
    EXPECT_EQ(lexical_errors, 0);
    EXPECT_EQ(syntax_errors, 1);
}




TEST_F(parser_test, lexerr_bad_string) {
    run_lexerr("lexerr_bad_string.lang");
    EXPECT_EQ(lexical_errors, 1);
    EXPECT_EQ(syntax_errors, 0);
}

TEST_F(parser_test, lexerr_bad_symb) {
    run_lexerr("lexerr_bad_symb.lang");
    EXPECT_EQ(lexical_errors, 2);
}

TEST_F(parser_test, lexerr_char) {
    run_lexerr("lexerr_char.lang");
    EXPECT_EQ(lexical_errors, 1);
    EXPECT_EQ(syntax_errors, 0);
}
