#pragma once

#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>

#include "ir_structs.hpp"

namespace ir {

    inline std::string type_str(const ast::type& t) {
        switch (t.kind_) {
            case ast::type_kind::I32:  return "i32";
            case ast::type_kind::BOOL: return "bool";
            case ast::type_kind::CHAR: return "char";
            case ast::type_kind::VOID: return "void";
            case ast::type_kind::POINTER:
                return "ptr<" + (t.base_ ? type_str(*t.base_) : "?") + ">";
            case ast::type_kind::ARRAY:
                return "arr<" + (t.base_ ? type_str(*t.base_) : "?") + ">";
        }
        return "?";
    }

    inline const char* op_name(op_code c) {
        switch (c) {
            case op_code::CONST: return "const";
            case op_code::ADD: return "add";
            case op_code::SUB: return "sub";
            case op_code::MUL: return "mul";
            case op_code::DIV: return "div";
            case op_code::AND: return "and";
            case op_code::OR:  return "or";
            case op_code::XOR: return "xor";
            case op_code::EQ:  return "eq";
            case op_code::NE:  return "ne";
            case op_code::LT:  return "lt";
            case op_code::LE:  return "le";
            case op_code::GT:  return "gt";
            case op_code::GE:  return "ge";
            case op_code::LOG_AND: return "logand";
            case op_code::LOG_OR:  return "logor";
            case op_code::NEG: return "neg";
            case op_code::NOT: return "not";
            case op_code::ASSIGN: return "assign";
            case op_code::ADDR:   return "addr";
            case op_code::LOAD:   return "load";
            case op_code::STORE:  return "store";
            case op_code::ELEM_PTR: return "elem_ptr";
            case op_code::CAST:   return "cast";
            case op_code::LABEL:  return "label";
            case op_code::JUMP:   return "jump";
            case op_code::BRANCH: return "branch";
            case op_code::CALL:   return "call";
            case op_code::RET:    return "ret";
        }
        return "?";
    }

    inline void dump_op(std::ostream& os, const operation& op) {
        os << "  ";
        if (op.result_) os << op.result_->name_ << " = ";
        os << op_name(op.code_);

        switch (op.code_) {
            case op_code::LABEL:
            case op_code::JUMP:
                os << " " << op.label_;
                break;
            case op_code::BRANCH:
                os << " " << (op.args_.empty() ? "?" : op.args_[0]->name_)
                   << ", " << op.true_label_ << ", " << op.false_label_;
                break;
            case op_code::CONST: {
                os << " ";
                const auto& c = op.const_value_;
                switch (c.type_.kind_) {
                    case ast::type_kind::I32:
                        os << c.int_value_;
                        break;
                    case ast::type_kind::BOOL:
                        os << (c.bool_value_ ? "true" : "false");
                        break;
                    case ast::type_kind::CHAR:
                        os << "'" << c.char_value_ << "'";
                        break;
                    case ast::type_kind::POINTER:  // строковый литерал — ptr<char>
                        os << "\"" << c.string_value_ << "\"";
                        break;
                    default:
                        os << c.int_value_;
                        break;
                }
                break;
            }
            case op_code::CALL: {
                os << " " << op.callee_name_ << "(";
                for (size_t i = 0; i < op.varargs_.size(); ++i) {
                    if (i) os << ", ";
                    os << op.varargs_[i]->name_;
                }
                os << ")";
                break;
            }
            default:
                if (!op.args_.empty())
                    os << " ";
                for (size_t i = 0; i < op.args_.size(); ++i) {
                    if (i) os << ", ";
                    os << op.args_[i]->name_;
                }
                break;
        }
        os << "\n";
    }

    inline void dump(std::ostream& os, const translation_unit& unit) {
        for (const auto& f : unit.functions_) {
            os << "function " << f->name_ << "() -> "
               << type_str(f->return_type_) << " {\n";
            for (const auto& op : f->operations_) dump_op(os, *op);
            os << "}\n\n";
        }
    }

    inline void dump_cfg(std::ostream& os, const function& f) {
        os << "CFG for " << f.name_ << ":\n";
        for (const auto& b : f.blocks_) {
            os << "  block " << b->label_ << ":\n";
            for (const auto& op : b->operations_)
                dump_op(os, *op);
            os << "    -> ";
            for (auto* s : b->successors_)
                os << s->label_ << " ";
            os << "\n";
        }
    }


    inline std::string escape_dot(const std::string& s) {
        std::string r;
        for (char c : s) {
            switch (c) {
                case '"':  r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\l";  break;
                case '\r': r += "";     break;
                case '\t': r += "  ";   break;
                default:   r += c;      break;
            }
        }
        return r;
    }

    inline std::string op_str(const operation& op) {
        std::ostringstream oss;
        dump_op(oss, op);
        std::string s = oss.str();
        // убираем ведущие пробелы и хвостовой \n
        while (!s.empty() && (s.front() == ' ')) s.erase(s.begin());
        while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
        return s;
    }

    inline void dump_cfg_dot(std::ostream& os, const function& f) {
        os << "digraph CFG_" << f.name_ << " {\n";
        os << "  rankdir=TB;\n";
        os << "  node [shape=box, fontname=\"monospace\", fontsize=10];\n";
        os << "  edge [fontname=\"monospace\", fontsize=9];\n";
        os << "\n";

        // Узлы-блоки
        for (const auto& b : f.blocks_) {
            std::string label;

            label += escape_dot(b->label_);
            label += "\\l";

            for (const auto& op : b->operations_) {
                if (op->code_ == op_code::LABEL) continue;
                label += escape_dot(op_str(*op));
                label += "\\l";
            }

            os << "  \"" << escape_dot(b->label_) << "\" [label=\""
               << label << "\"];\n";
        }

        os << "\n";

        for (const auto& b : f.blocks_) {
            if (b->operations_.empty()) continue;
            auto last = b->operations_.back();

            for (auto* s : b->successors_) {
                std::string edge_label;
                if (last->code_ == op_code::BRANCH) {
                    if (s->label_ == last->true_label_)  edge_label = "true";
                    if (s->label_ == last->false_label_) edge_label = "false";
                }
                if (!edge_label.empty()) {
                    os << "  \"" << escape_dot(b->label_) << "\" -> \""
                       << escape_dot(s->label_) << "\" [label=\""
                       << edge_label << "\"];\n";
                } else {
                    os << "  \"" << escape_dot(b->label_) << "\" -> \""
                       << escape_dot(s->label_) << "\";\n";
                }
            }
        }

        os << "}\n";
    }
    inline void open_cfg_png(const function& f) {
    // уникальное имя по времени и pid
    std::string base = "/tmp/cfg_" + f.name_ + "_"
                     + std::to_string(std::time(nullptr)) + "_"
                     + std::to_string(::getpid());
    std::string dot_path = base + ".dot";
    std::string png_path = base + ".png";

    // 1. пишем DOT
    {
        std::ofstream out(dot_path);
        if (!out) {
            std::cerr << "Cannot write " << dot_path << "\n";
            return;
        }
        dump_cfg_dot(out, f);
    }

    // 2. dot -> png
    std::string cmd = "dot -Tpng " + dot_path + " -o " + png_path;
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "dot failed for " << f.name_ << "\n";
        std::remove(dot_path.c_str());
        return;
    }

    // 3. открываем PNG в фоне, не блокируя
    cmd = "xdg-open " + png_path + " >/dev/null 2>&1 &";
    std::system(cmd.c_str());

    // 4. удаляем .dot, png оставляем
    std::remove(dot_path.c_str());
}
}