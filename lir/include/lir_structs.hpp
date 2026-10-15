#pragma once
#include "ast.hpp"

/**
* Задание на практику: низкоуровневое представление.
1. Объекты становятся смещениями в памяти относительно начала кадра стека или вовсе существуют только в регистрах (условие: помещаются в регистры и на них не брали указателей).
2. У вас есть регистры sp (вершина стека), bp (начало кадра стека) и виртуальные регистры.
3. Все сложные операции транслируются в цепочки более простых (по типу ассемблера). Например, обращение к полю структуры станет ld/st по вычисленному адресу.
 */
namespace lir {
    // тип операнда
    enum class operand_kind {
        REG,        // виртуальный регистр: %r0, %r1, %r2 и тд
        IMM,        // immediate - константа: 5, true, 'a' и тд
        STACK,      // операнд, который лежит в стеке [bp + offset]
        GLOBAL,     // глобальный объект
        LABEL       // label - метка
    };

    // операнд
    struct operand {
        operand_kind kind_ = operand_kind::REG;
        std::string name_;
        long long imm_ = 0;
        int offset_ = 0;
        ast::type type_;

        static operand reg(const std::string& name, const ast::type& type) {
            operand op;
            op.kind_ = operand_kind::REG;
            op.name_ = name;
            op.type_ = type;
            return op;
        }
        static operand imm(const long long val, const ast::type& type) {
            operand op;
            op.kind_ = operand_kind::IMM;
            op.imm_ = val;
            op.type_ = type;
            return op;
        }
        static operand stack(int off, const ast::type& type) {
            operand op;
            op.kind_ = operand_kind::STACK;
            op.offset_ = off;
            op.type_ = type;
            return op;
        }
        static operand global(const std::string& name, const ast::type& type) {
            operand op;
            op.kind_ = operand_kind::GLOBAL;
            op.name_ = name;
            op.type_ = type;
            return op;
        }
        static operand label(const std::string& name) {
            operand op;
            op.kind_ = operand_kind::LABEL;
            op.name_ = name;
            return op;
        }
    };

    // тип операции
    enum class lir_op {
        ENTER, LEAVE,
        MOV, LD, ST, LEA,
        PUSH, POP,
        ADD, SUB, MUL, DIV,
        AND, OR, XOR,
        NEG, NOT,
        CMP,
        SET_EQ, SET_NE, SET_LT, SET_LE, SET_GT, SET_GE,
        JMP, JZ, JNZ,
        CALL, RET,
        LABEL
    };

    // структура инструкции
    struct lir_instruction {
        lir_op code_;
        std::vector<operand> operands_;
        size_t line_ = 0;
    };

    // структура, которая говорит о месте, в котором хранится объект
    enum class slot_kind {
        REGISTER,
        STACK,
        GLOBAL
    };

    struct frame_slot {
        std::string hir_name_;
        slot_kind kind_;
        std::string reg_name_;      // это если REGISTER
        int offset_ = 0;            // это если STACK
        ast::type type_;
    };


    struct lir_function {
        std::string name_;
        ast::type return_type_;
        std::vector<lir_instruction> instructions_;
        std::vector<frame_slot> frame_;
        size_t frame_size_ = 0;
    };

    struct lir_unit {
        std::vector<lir_function> functions_;
    };
}