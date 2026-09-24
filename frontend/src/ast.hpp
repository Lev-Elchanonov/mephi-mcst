#pragma once
#include <memory>
#include <vector>




namespace ast {
    enum class type_kind {
        I32, BOOL, CHAR, VOID, ARRAY, POINTER   // типы данных
    };

    struct type {
        type_kind kind_ = type_kind::VOID;      // вид типа
        std::shared_ptr<type> base_;            // для ARRAY и POINTER нужен тип, на которую они указывают
        size_t array_size_ = 0;                 // для ARRAY длина
    };

    enum class binary_op {  // бинарные операторы
        ADD, SUB, MUL, DIV,
        BIN_AND, BIN_OR,
        EQ, NE, LT, LE, GT, GE,
        LOG_AND, LOG_OR
    };

    enum class unary_op {   // унарные операторы
        NEG, LOG_NOT
    };

    enum class expr_kind {  // выражения
        INT_LIT, BOOL_LIT, CHAR_LIT, STRING_LIT,
        VAR_REF, BIN_OP, UN_OP, CALL, INDEX
    };

    // ВЫРАЖЕНИЕ : это то, что имеет значение (можно вычислить и получить результат)
    struct expr {
        expr_kind kind_;   // тип выражения
        type type_;
        size_t line_;      // строка в коде
        size_t column_;    // колонка в коде

        long long int_value_; // значение int
        bool bool_value_;     // значение bool
        char char_value_;     // значение char
        std::string string_value_; // значение строки

        std::string name_;  // имя переменной

        binary_op bin_op_ = binary_op::ADD;  // унарная операция
        unary_op un_op_ = unary_op::NEG;    // бинарная операция

        std::shared_ptr<expr> lhs_;
        std::shared_ptr<expr> rhs_;
        std::vector<std::shared_ptr<expr>> args_;   // список аргументов
    };

    using expr_ptr = std::shared_ptr<expr>;

    enum class stmt_kind {
        VAR_DECL,   // i32 a
        ASSIGN,     // a = c
        ASSIGN_OP,  // a += b
        IF,
        WHILE,
        RETURN,
        EXPR_STMT,  // выражение как инструкция (foo())
        BLOCK,      // блок конструкции в {}
        BREAK,
        CONTINUE
    };

    // КОНСТРУКЦИЯ : это то, что выполняется, но не возвращает результат (например int x = 10)
    struct stmt {
        stmt_kind kind_;    // вид
        size_t line_;       // строка в коде
        size_t column_;     // колонка в коде

        type var_type_;     // тип объявления переменной
        std::string name_;  // имя
        expr_ptr init_value_; // init выражение (может быть null или результат expr)

        expr_ptr target_;   // куда присваиваем
        expr_ptr value_;    // что присваиваем
        binary_op assign_op_ = binary_op::ADD; // какая операция

        expr_ptr condition_; // условия
        std::vector<std::shared_ptr<stmt>> then_body_; // если условие if истинно
        std::vector<std::shared_ptr<stmt>> else_body_; // если условие if ложно


        expr_ptr return_value_; // возвращаемое значение (mb nullptr)

        expr_ptr expr_;         // выражение-конструкция

        std::vector<std::shared_ptr<stmt>> body_; // инструкции блока
    };

    using stmt_ptr = std::shared_ptr<stmt>;

    // параметры функции
    struct param {
        type type_;         // тип параметра
        std::string name_;  // имя параметра
    };

    // объявление функции
    struct func_decl {
        type return_type_;  // возвращаемый тип
        std::string name_;  // имя функции
        std::vector<param> params_; // параметры функции
        std::vector<stmt_ptr> body_; // тело функции
        size_t line_ = 0;           // строка в коде
        size_t column_ = 0;         // столбец в коде
    };

    // вся программа (список функций)
    struct program {
        std::vector<std::shared_ptr<func_decl>> functions_; // все функции, что есть в программе
    };
}