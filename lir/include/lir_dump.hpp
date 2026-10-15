#pragma once

#include "lir_structs.hpp"

namespace lir {

    inline const char* lir_op_name(lir_op c) {
        switch (c) {
            case lir_op::ENTER: return "enter";
            case lir_op::LEAVE: return "leave";
            case lir_op::MOV:   return "mov";
            case lir_op::LD:    return "ld";
            case lir_op::ST:    return "st";
            case lir_op::LEA:   return "lea";
            case lir_op::PUSH:  return "push";
            case lir_op::POP:   return "pop";
            case lir_op::ADD:   return "add";
            case lir_op::SUB:   return "sub";
            case lir_op::MUL:   return "mul";
            case lir_op::DIV:   return "div";
            case lir_op::AND:   return "and";
            case lir_op::OR:    return "or";
            case lir_op::XOR:   return "xor";
            case lir_op::NEG:   return "neg";
            case lir_op::NOT:   return "not";
            case lir_op::CMP:   return "cmp";
            case lir_op::JMP:   return "jmp";
            case lir_op::JZ:    return "jz";
            case lir_op::JNZ:   return "jnz";
            case lir_op::CALL:  return "call";
            case lir_op::RET:   return "ret";
            case lir_op::LABEL: return "label";
            case lir_op::SET_EQ: return "set_eq";
            case lir_op::SET_NE: return "set_ne";
            case lir_op::SET_LT: return "set_lt";
            case lir_op::SET_LE: return "set_le";
            case lir_op::SET_GT: return "set_gt";
            case lir_op::SET_GE: return "set_ge";
        }
        return "?";
    }

    inline std::string op_str(const operand& o) {
        switch (o.kind_) {
            case operand_kind::REG:    return o.name_;
            case operand_kind::IMM:    return std::to_string(o.imm_);
            case operand_kind::STACK:  return "[bp" + std::string(o.offset_ >= 0 ? "+" : "") + std::to_string(o.offset_) + "]";
            case operand_kind::GLOBAL: return "@" + o.name_;
            case operand_kind::LABEL:  return o.name_;
        }
        return "?";
    }

    inline void dump_lir_function(std::ostream& os, const lir_function& f) {
        os << "function " << f.name_ << ":\n";
        os << "  frame: " << f.frame_size_ << " bytes\n";
        for (const auto& s : f.frame_) {
            os << "    " << s.hir_name_ << " -> ";
            if (s.kind_ == slot_kind::REGISTER) os << "reg " << s.reg_name_;
            else if (s.kind_ == slot_kind::STACK) os << "stack [bp" << (s.offset_ >= 0 ? "+" : "") << s.offset_ << "]";
            else os << "global @" << s.hir_name_;
            os << "\n";
        }
        os << "\n";
        for (const auto& i : f.instructions_) {
            if (i.code_ == lir_op::LABEL) {
                os << op_str(i.operands_[0]) << ":\n";
                continue;
            }
            os << "  " << lir_op_name(i.code_);
            for (size_t k = 0; k < i.operands_.size(); ++k)
                os << (k ? ", " : " ") << op_str(i.operands_[k]);
            os << "\n";
        }
        os << "\n";
    }

    inline void dump_lir(std::ostream& os, const lir_unit& unit) {
        for (const auto& f : unit.functions_)
            dump_lir_function(os, f);
    }

}