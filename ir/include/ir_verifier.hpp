#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>

#include "ir_structs.hpp"

namespace ir {
    class verifier {
        std::vector<std::string> errors_;

        void error(std::size_t line, const std::string& msg) {
            errors_.push_back("line " + std::to_string(line) + ": " + msg);
        }
    public:
        bool verify(const translation_unit& unit) {
            errors_.clear();
            for (const auto& func : unit.functions_)
                verify_func(*func);

            for (const auto& e : errors_) {
                std::cerr << "IR ERROR: " << e << std::endl;
            }
            return errors_.empty();
        }
        const std::vector<std::string>& get_errors() const { return errors_; }
    private:
        void verify_func(const function& f) {
            std::unordered_set<std::string> defined;
            std::unordered_set<std::string> labels;
            std::vector<std::string> used_labels;

            // все метки
            for (const auto& op : f.operations_) {
                if (op->code_ == op_code::LABEL) {
                    if (labels.count(op->label_)) {
                        error(op->line_, "duplicate label '" + op->label_ + "'");
                    }
                    labels.insert(op->label_);
                }
            }

            for (const auto& p : f.params_)
                defined.insert(p->name_);

            for (const auto& op : f.operations_) {
                verify_op(*op, defined, used_labels);
                if (op->result_)
                    defined.insert(op->result_->name_);
            }

            for (const auto& l : used_labels) {
                if (!labels.count(l)) {
                    error(0, "undefined label '" + l + "'");
                }
            }
        }

        void verify_op(const operation& op, const std::unordered_set<std::string>& defined, std::vector<std::string>& used_labels) {
            auto check_obj = [&](const object_ptr& a) {
                if (a && a->kind_ == obj_kind::TEMP && !defined.count(a->name_)) {
                    error(op.line_, "value '" + a->name_ + "' used before defined");
                }
            };

            for (const auto& a : op.args_) check_obj(a);
            for (const auto& a : op.varargs_) check_obj(a);

            switch (op.code_) {
                case op_code::ADD:
                case op_code::SUB:
                case op_code::MUL:
                case op_code::DIV:
                case op_code::AND:
                case op_code::OR:
                    if (op.args_.size() != 2) {
                        error(op.line_, "binary op needs 2 args");
                        break;
                    }
                    if (!is_num(op.args_[0]->type_) || !is_num(op.args_[1]->type_)) {
                        error(op.line_, "binary op requires numeric operands");
                    }
                    break;

                case op_code::BRANCH:
                    if (op.args_.size() != 1) {
                        error(op.line_, "branch needs 1 arg");
                        break;
                    }
                    if (op.args_[0]->type_.kind_ != ast::type_kind::BOOL) {
                        error(op.line_, "branch condition must be bool");
                    }
                    used_labels.push_back(op.true_label_);
                    used_labels.push_back(op.false_label_);
                    break;

                case op_code::JUMP:
                    used_labels.push_back(op.label_);
                    break;

                default:
                    break;
            }

        }
        static bool is_num(const ast::type& t) {
            return t.kind_ == ast::type_kind::I32  ||
                   t.kind_ == ast::type_kind::CHAR ||
                   t.kind_ == ast::type_kind::BOOL;
        }

    };


}