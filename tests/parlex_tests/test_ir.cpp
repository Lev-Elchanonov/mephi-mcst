#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

#include "lang_lexer.hpp"
#include "parser.tab.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "ir_structs.hpp"
#include "ir_builder.hpp"
#include "ir_verifier.hpp"
#include "ir_cfg_builder.hpp"

extern std::shared_ptr<ast::program> program;
extern int syntax_errors;
extern int lexical_errors;

using ast_builder = ir::builder<ast::program, ast::func_decl, ast::stmt, ast::expr>;

class ir_test : public ::testing::Test {
protected:
    ir::translation_unit unit_;

    void SetUp() override {
        program = std::make_shared<ast::program>();
        syntax_errors = 0;
        lexical_errors = 0;
    }

    void TearDown() override {
        program.reset();
    }

    // Прогон файла через лексер, парсер, semantic и IR
    // Возвращает true, если всё ок
    bool run(const std::string& dir, const std::string& filename) {
        std::string path = std::string(dir) + "/" + filename;
        std::ifstream file(path);
        if (!file) {
            ADD_FAILURE() << "Cannot open " << path;
            return false;
        }

        yy::lang_lexer lexer(file);
        yy::Parser parser(lexer);
        parser.parse();

        if (lexical_errors || syntax_errors) return false;

        sema::analyzer a;
        if (!a.analyze(program)) return false;

        ast_builder b;
        b.build(program, unit_);
        return true;
    }

    // Считает операции заданного вида во всех функциях
    static int count_op(const ir::translation_unit& unit, ir::op_code code) {
        int n = 0;
        for (const auto& f : unit.functions_)
            for (const auto& op : f->operations_)
                if (op->code_ == code) ++n;
        return n;
    }

    // Считает операции вида в конкретной функции
    static int count_op_in(const ir::function& f, ir::op_code code) {
        int n = 0;
        for (const auto& op : f.operations_)
            if (op->code_ == code) ++n;
        return n;
    }

    // Ищет функцию по имени
    static const ir::function* find_func(const ir::translation_unit& unit,
                                          const std::string& name) {
        for (const auto& f : unit.functions_)
            if (f->name_ == name) return f.get();
        return nullptr;
    }

    // Ищет метку по имени среди всех операций функции
    static bool has_label(const ir::function& f, const std::string& label) {
        for (const auto& op : f.operations_)
            if (op->code_ == ir::op_code::LABEL && op->label_ == label)
                return true;
        return false;
    }
};



TEST_F(ir_test, empty_program) {
    program->functions_.clear();
    ast_builder b;
    b.build(program, unit_);

    EXPECT_TRUE(unit_.functions_.empty());
    EXPECT_TRUE(unit_.globals_.empty());
}

TEST_F(ir_test, simple_main) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_minim.lang"));

    auto* main_f = find_func(unit_, "main");
    ASSERT_NE(main_f, nullptr);

    // return 0 → const + ret
    EXPECT_GE(count_op_in(*main_f, ir::op_code::CONST), 1);
    EXPECT_GE(count_op_in(*main_f, ir::op_code::RET), 1);
}

TEST_F(ir_test, globals_for_functions) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    // для каждой функции создан глобальный объект
    EXPECT_EQ(unit_.globals_.size(), program->functions_.size());

    // проверим, что у глобалов is_function_ = true
    for (const auto& g : unit_.globals_) {
        EXPECT_TRUE(g->is_function_);
        EXPECT_EQ(g->kind_, ir::obj_kind::GLOBAL);
    }
}

TEST_F(ir_test, parameters_are_registered) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    auto* main_f = find_func(unit_, "main");
    ASSERT_NE(main_f, nullptr);

    // у main 2 параметра: argc, argv (и ptr$ ptr$ char)
    EXPECT_EQ(main_f->params_.size(), 2);
    EXPECT_EQ(main_f->params_[0]->name_, "argc");
    EXPECT_EQ(main_f->params_[1]->name_, "argv");
}


// ТУТ ВЫРАЖЕНИЯ
TEST_F(ir_test, const_generates_const_pp) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_minim.lang"));

    auto* main_f = find_func(unit_, "main");
    ASSERT_NE(main_f, nullptr);

    EXPECT_GE(count_op_in(*main_f, ir::op_code::CONST), 1);
}

TEST_F(ir_test, binary_op_generatesOp) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_operations.lang"));

    // где-то должны быть арифметические операции
    int arith = count_op(unit_, ir::op_code::ADD)
              + count_op(unit_, ir::op_code::SUB)
              + count_op(unit_, ir::op_code::MUL)
              + count_op(unit_, ir::op_code::DIV);
    EXPECT_GT(arith, 0);
}

TEST_F(ir_test, call_generates_call) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    auto* main_f = find_func(unit_, "main");
    ASSERT_NE(main_f, nullptr);

    // prints, printi — вызовы
    EXPECT_GT(count_op_in(*main_f, ir::op_code::CALL), 0);
}

TEST_F(ir_test, index_generates_elem_ptr) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_arrays.lang"));

    EXPECT_GT(count_op(unit_, ir::op_code::ELEM_PTR), 0);
    EXPECT_GT(count_op(unit_, ir::op_code::LOAD), 0);
}


// УПРАВЯЛЮЩИЕ КОНСТРУКЦИИ
TEST_F(ir_test, if_generates_branch_and_labels) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_ifelse.lang"));

    int branches = count_op(unit_, ir::op_code::BRANCH);
    EXPECT_GT(branches, 0);

    // у каждой ветки должны быть метки
    auto* f = unit_.functions_.front().get();
    int then_labels = 0;
    int else_labels = 0;
    int end_labels = 0;
    for (const auto& op : f->operations_) {
        if (op->code_ != ir::op_code::LABEL) continue;
        if (op->label_.rfind("if_then_", 0) == 0) ++then_labels;
        if (op->label_.rfind("if_else_", 0) == 0) ++else_labels;
        if (op->label_.rfind("if_end_", 0) == 0) ++end_labels;
    }
    EXPECT_GT(then_labels, 0);
    EXPECT_GT(else_labels, 0);
    EXPECT_GT(end_labels, 0);
}

TEST_F(ir_test, while_generates_jump_and_labels) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    auto* main_f = find_func(unit_, "main");
    ASSERT_NE(main_f, nullptr);

    EXPECT_GT(count_op_in(*main_f, ir::op_code::BRANCH), 0);
    EXPECT_GT(count_op_in(*main_f, ir::op_code::JUMP), 0);

    // есть метки while_*
    bool has_while_start = false;
    bool has_while_body = false;
    bool has_while_end = false;
    for (const auto& op : main_f->operations_) {
        if (op->code_ != ir::op_code::LABEL) continue;
        if (op->label_.rfind("while_", 0) == 0 &&
            op->label_.rfind("while_body_", 0) != 0 &&
            op->label_.rfind("while_end_", 0) != 0)
            has_while_start = true;
        if (op->label_.rfind("while_body_", 0) == 0) has_while_body = true;
        if (op->label_.rfind("while_end_", 0) == 0) has_while_end = true;
    }
    EXPECT_TRUE(has_while_start);
    EXPECT_TRUE(has_while_body);
    EXPECT_TRUE(has_while_end);
}
/*
TEST_F(ir_test, break_generates_jump_to_end_label) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_nested.lang"));

    auto* f = unit_.functions_.front().get();
    bool found_jump_to_while_end = false;
    for (const auto& op : f->operations_) {
        if (op->code_ == ir::op_code::JUMP &&
            op->label_.rfind("while_end_", 0) == 0) {
            found_jump_to_while_end = true;
        }
    }
    // если в программе есть break, будет jump на while_end
    // в valid_nested нет break, поэтому проверяем, что ошибок верификатора нет
    SUCCEED();
}
*/

// тут verifier
TEST_F(ir_test, verifier_accepts_valid) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::verifier v;
    EXPECT_TRUE(v.verify(unit_));
    EXPECT_TRUE(v.get_errors().empty());
}

TEST_F(ir_test, verifier_accepts_minim) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_minim.lang"));

    ir::verifier v;
    EXPECT_TRUE(v.verify(unit_));
}

TEST_F(ir_test, verifier_accepts_arrays) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_arrays.lang"));

    ir::verifier v;
    EXPECT_TRUE(v.verify(unit_));
}

TEST_F(ir_test, verifier_accepts_nested) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_nested.lang"));

    ir::verifier v;
    EXPECT_TRUE(v.verify(unit_));
}

TEST_F(ir_test, verifier_accepts_full) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_full.lang"));

    ir::verifier v;
    EXPECT_TRUE(v.verify(unit_));
}

// CFG
TEST_F(ir_test, cfg_has_entry) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);
        EXPECT_NE(f->entry_, nullptr);
        EXPECT_FALSE(f->blocks_.empty());
    }
}

TEST_F(ir_test, cfg_blocks_have_labels) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);
        for (const auto& b : f->blocks_) {
            EXPECT_FALSE(b->label_.empty());
        }
    }
}

TEST_F(ir_test, cfg_branch_has_two_succ) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs","valid_ifelse.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            if (b->operations_.empty()) continue;
            auto last = b->operations_.back();
            if (last->code_ == ir::op_code::BRANCH) {
                EXPECT_EQ(b->successors_.size(), 2);
            }
        }
    }
}

TEST_F(ir_test, cfg_jump_has_one_succ) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            if (b->operations_.empty()) continue;
            auto last = b->operations_.back();
            if (last->code_ == ir::op_code::JUMP) {
                EXPECT_EQ(b->successors_.size(), 1);
            }
        }
    }
}

TEST_F(ir_test, cfg_ret_has_no_succ) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_minim.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            if (b->operations_.empty()) continue;
            auto last = b->operations_.back();
            if (last->code_ == ir::op_code::RET) {
                EXPECT_TRUE(b->successors_.empty());
            }
        }
    }
}

TEST_F(ir_test, cfg_successors_are_in_blocks) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            for (auto* s : b->successors_) {
                // каждый последователь — один из блоков этой функции
                bool found = false;
                for (const auto& other : f->blocks_) {
                    if (other.get() == s) { found = true; break; }
                }
                EXPECT_TRUE(found);
            }
        }
    }
}

TEST_F(ir_test, cfg_predecessors_are_in_blocks) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            for (auto* p : b->predecessors_) {
                bool found = false;
                for (const auto& other : f->blocks_) {
                    if (other.get() == p) { found = true; break; }
                }
                EXPECT_TRUE(found);
            }
        }
    }
}

TEST_F(ir_test, cfg_edges_are_symmetric) {
    ASSERT_TRUE(run("tests/parlex_tests/programs/valid_programs", "valid_example.lang"));

    ir::cfg_builder cfg;
    for (auto& f : unit_.functions_) {
        cfg.build(*f);

        for (const auto& b : f->blocks_) {
            for (auto* s : b->successors_) {
                // если b → s, то s должен содержать b в predecessors_
                bool found = false;
                for (auto* p : s->predecessors_) {
                    if (p == b.get()) { found = true; break; }
                }
                EXPECT_TRUE(found) << "edge " << b->label_ << " -> " << s->label_
                                   << " not symmetric";
            }
        }
    }
}