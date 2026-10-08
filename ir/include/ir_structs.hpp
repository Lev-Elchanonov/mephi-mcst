#pragma once

#include "ast.hpp"


namespace ir {
    struct function;
    struct basic_block;

    using object_ptr = std::shared_ptr<struct object>;
    using operation_ptr = std::shared_ptr<struct operation>;


    enum class obj_kind {
        LOCAl,  // локальная переменная
        PARAM,  // параметр функции
        TEMP,   // временное значение (результат операции)
        GLOBAL, // глобальный объект
    };

    struct constant {
        ast::type type_;
        long long int_value_;
        bool bool_value_;
        char char_value_;
        std::string string_value_;
    };

    struct object {
        std::string name_;
        ast::type type_;
        obj_kind kind_;

        bool is_function_;
        constant const_value_;
    };

    enum struct op_code {
        CONST,
        ADD, SUB, MUL, DIV,
        AND, OR, XOR,
        EQ, NE, LT, LE, GT, GE,
        LOG_AND, LOG_OR,
        NEG, NOT,
        ASSIGN, ADDR, LOAD, STORE, ELEM_PTR,
        CAST,
        LABEL, JUMP, BRANCH, CALL, RET
    };

    struct operation {
        op_code code_;
        size_t line_ = 0;

        std::vector<object_ptr> args_;      // фиксированные аргументы (до 3)
        std::vector<object_ptr> varargs_;   // список переменной длины (для CALL)
        object_ptr result_;                 // куда кладём результат

        std::string label_;                // для LABEL, JUMP
        std::string true_label_;           // для BRANCH
        std::string false_label_;
        std::string callee_name_;          // для CALL по имени

        constant const_value_;             // для CONS
    };

    struct basic_block {
        std::string label_;
        std::vector<operation_ptr> operations_;
        std::vector<basic_block*> successors_;      // последователи
        std::vector<basic_block*> predecessors_;    // предшественники
    };

    struct function {
        std::string name_;
        ast::type return_type_;
        std::vector<object_ptr> params_;
        std::vector<object_ptr> locals_;
        std::vector<operation_ptr> operations_;

        std::vector<std::unique_ptr<basic_block>> blocks_;
        basic_block* entry_ = nullptr;
    };

    struct translation_unit {
        std::vector<object_ptr> globals_;
        std::vector<std::unique_ptr<function>> functions_;
    };
}