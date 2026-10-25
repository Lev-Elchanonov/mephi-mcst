#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "lir_structs.hpp"

namespace cgen {

class c_generator {
public:
    static constexpr const char* IND = "    ";
    void generate(const lir::lir_unit& unit, std::ostream& out) {
        out << "#include \"vm.h\"\n\n";

        for (const auto& f : unit.functions_) {
            for (const auto& s : f.frame_) {
                if (s.kind_ == lir::slot_kind::STACK) {
                    int bytes = lir::align8(lir::type_size(s.type_));
                    int n = (bytes > 8) ? bytes / 8 : 1;
                    if (n == 1)
                        out << "uint64_t stack_" << f.name_ << "_" << s.hir_name_ << ";\n";
                    else
                        out << "uint64_t stack_" << f.name_ << "_" << s.hir_name_ << "[" << n << "];\n";
                }
            }
        }
        out << "\n";

        for (const auto& f : unit.functions_) {
            generate_function(f, out);
        }
    }

private:
    void generate_function(const lir::lir_function& f, std::ostream& out) {
        std::string name = (f.name_ == "main") ? "__main__" : "__" + f.name_ + "__";
        out << "void " << name << "(void) {\n";

        for (const auto& instr : f.instructions_) {
            generate_instruction(instr, f, out);
        }

        out << "}\n\n";
    }

    void generate_instruction(const lir::lir_instruction& instr, const lir::lir_function& f, std::ostream& out) {
        using lir::lir_op;

        switch (instr.code_) {
            case lir_op::ENTER:
            case lir_op::LEAVE:
                break;

            case lir_op::MOV: {
                if (instr.operands_.size() < 2) break;
                out << IND << "MOV(" << op_to_expr(instr.operands_[0], f)
                    << ", " << op_to_expr(instr.operands_[1], f) << ");\n";
                break;
            }

            case lir_op::LD: {
                if (instr.operands_.size() < 2) break;
                out << IND << "LOAD(" << op_to_expr(instr.operands_[0], f)
                    << ", " << op_to_expr(instr.operands_[1], f) << ", 0);\n";
                break;
            }

            case lir_op::ST: {
                if (instr.operands_.size() < 2) break;
                out << IND << "STORE(" << op_to_expr(instr.operands_[0], f)
                    << ", 0, " << op_to_expr(instr.operands_[1], f) << ");\n";
                break;
            }

            case lir_op::LEA: {
                if (instr.operands_.size() < 2) break;
                out << IND << "LEA(" << op_to_expr(instr.operands_[0], f)
                    << ", " << op_to_expr(instr.operands_[1], f) << ", 0);\n";
                break;
            }

            case lir_op::ADD:
            case lir_op::SUB:
            case lir_op::MUL:
            case lir_op::DIV:
            case lir_op::AND:
            case lir_op::OR:
            case lir_op::XOR: {
                if (instr.operands_.size() < 3) break;
                out << IND << op_name(instr.code_) << "("
                    << op_to_expr(instr.operands_[0], f) << ", "
                    << op_to_expr(instr.operands_[1], f) << ", "
                    << op_to_expr(instr.operands_[2], f) << ");\n";
                break;
            }

            case lir_op::NEG:
            case lir_op::NOT: {
                if (instr.operands_.size() < 2) break;
                out << IND << op_name(instr.code_) << "("
                    << op_to_expr(instr.operands_[0], f) << ", "
                    << op_to_expr(instr.operands_[1], f) << ");\n";
                break;
            }

            case lir_op::SET_EQ:
            case lir_op::SET_NE:
            case lir_op::SET_LT:
            case lir_op::SET_LE:
            case lir_op::SET_GT:
            case lir_op::SET_GE: {
                if (instr.operands_.size() < 3) break;
                out << IND << op_name(instr.code_) << "("
                    << op_to_expr(instr.operands_[0], f) << ", "
                    << op_to_expr(instr.operands_[1], f) << ", "
                    << op_to_expr(instr.operands_[2], f) << ");\n";
                break;
            }

            case lir_op::JMP: {
                out << IND << "JMP(" << instr.operands_[0].name_ << ");\n";
                break;
            }
            case lir_op::JZ: {
                out << IND << "JZ(" << op_to_expr(instr.operands_[0], f)
                    << ", " << instr.operands_[1].name_ << ");\n";
                break;
            }
            case lir_op::JNZ: {
                out << IND << "JNZ(" << op_to_expr(instr.operands_[0], f)
                    << ", " << instr.operands_[1].name_ << ");\n";
                break;
            }

            case lir_op::LABEL: {
                out << "LABEL(" << instr.operands_[0].name_ << ");\n";
                break;
            }

            case lir_op::CALL: {
                out << IND << "__" << instr.operands_[0].name_ << "__();\n";
                break;
            }

            case lir_op::RET: {
                out << IND << "return;\n";
                break;
            }

            case lir_op::PUSH: {
                out << IND << "PUSH(" << op_to_expr(instr.operands_[0], f) << ");\n";
                break;
            }
            case lir_op::POP: {
                out << IND << "POP();\n";
                break;
            }

            default:
                break;
        }
    }

    // Возвращает строку для подстановки в макрос
    std::string op_to_expr(const lir::operand& op, const lir::lir_function& f) {
        using lir::operand_kind;

        if (op.kind_ == operand_kind::REG) {
            std::string s = op.name_;
            if (s == "%rax" || s == "%ax") return "rax";
            if (s.size() > 2 && s[0] == '%' && s[1] == 'r') {
                return "r[" + s.substr(2) + "]"; // %r2 -> r[2]
            }
            return "r[0]";
        }
        if (op.kind_ == operand_kind::IMM) {
            return std::to_string(op.imm_); // константа
        }
        if (op.kind_ == operand_kind::STACK) {
            for (const auto& s : f.frame_) {
                if (s.kind_ == lir::slot_kind::STACK && s.offset_ == op.offset_) {
                    return "(&stack_" + f.name_ + "_" + s.hir_name_ + ")";   // ← &
                }
            }
            return "(&stack_" + f.name_ + "_off" + std::to_string(op.offset_) + ")";
        }
        if (op.kind_ == operand_kind::GLOBAL) {
            return op.name_;
        }
        return "0";
    }

    std::string op_name(lir::lir_op op) {
        using lir::lir_op;
        switch (op) {
            case lir_op::ADD: return "ADD";
            case lir_op::SUB: return "SUB";
            case lir_op::MUL: return "MUL";
            case lir_op::DIV: return "DIV";
            case lir_op::AND: return "AND";
            case lir_op::OR:  return "OR";
            case lir_op::XOR: return "XOR";
            case lir_op::NEG: return "NEG";
            case lir_op::NOT: return "NOT";
            case lir_op::SET_EQ: return "EQ";
            case lir_op::SET_NE: return "NE";
            case lir_op::SET_LT: return "LT";
            case lir_op::SET_LE: return "LE";
            case lir_op::SET_GT: return "GT";
            case lir_op::SET_GE: return "GE";
            default: return "?";
        }
    }
};

}