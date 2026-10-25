#pragma once
#include "ast.hpp"
#include "lir_structs.hpp"
#include <unordered_map>
#include <unordered_set>
#include "ir_structs.hpp"

namespace lir {
    inline int align8(int n) { return (n + 7) & ~7; }

    inline int type_size(const ast::type& t) {
        switch (t.kind_) {
            case ast::type_kind::I32:     return 4;
            case ast::type_kind::BOOL:    return 1;
            case ast::type_kind::CHAR:    return 1;
            case ast::type_kind::VOID:    return 0;
            case ast::type_kind::POINTER: return 8;
            case ast::type_kind::ARRAY:
                return t.array_size_ * (t.base_ ? type_size(*t.base_) : 1);
        }
        return 8;
    }

    class lowerer {
    private:
        lir_function* fn_ = nullptr;
        size_t vreg_counter_ = 0;
        int next_offset_ = 0;
        size_t frame_used_ = 0;
        std::unordered_map<std::string, frame_slot> slots_;
        std::unordered_set<std::string> address_taken_;

    public:
        void lower(const ir::translation_unit& unit, lir_unit& out) {
            for (const auto& f : unit.functions_) {
                lir_function lf;
                lower_function(*f, lf); // понижаем функцию
                out.functions_.push_back(std::move(lf)); // муваем ее в массив пониженных функций
            }
        }

    private:
        std::string new_vreg() { return "%r" + std::to_string(vreg_counter_++); }

        lir_instruction& emit(lir_op op, size_t line = 0) {
            lir_instruction i;
            i.code_ = op;
            i.line_ = line;
            fn_->instructions_.push_back(i);
            return fn_->instructions_.back();
        }

        void collect_address_taken(const ir::function& f) {
            address_taken_.clear();
            for (const auto& op : f.operations_) {
                if (op->code_ == ir::op_code::ADDR && !op->args_.empty()) {
                    address_taken_.insert(op->args_[0]->name_);
                }
            }
        }

        // во временные регистры можно помещать temp значения и те, что не подходят к другим типам
        bool can_be_in_reg(const ir::object& obj) {
            if (obj.kind_ == ir::obj_kind::GLOBAL) return false;
            if (obj.kind_ == ir::obj_kind::TEMP)   return true;
            if (obj.type_.kind_ == ast::type_kind::ARRAY) return false;
            if (address_taken_.count(obj.name_))   return false;
            return true;
        }

        // создание слота под объект
        frame_slot make_slot(const ir::object& obj) {
            frame_slot s;
            s.hir_name_ = obj.name_;
            s.type_ = obj.type_;

            if (obj.kind_ == ir::obj_kind::GLOBAL) {
                s.kind_ = slot_kind::GLOBAL;
                s.reg_name_ = "@" + obj.name_;
            } else if (can_be_in_reg(obj)) {
                s.kind_ = slot_kind::REGISTER;
                s.reg_name_ = new_vreg();
            } else {
                int sz = align8(type_size(obj.type_));
                next_offset_ -= sz;
                frame_used_ += sz;
                s.kind_ = slot_kind::STACK;
                s.offset_ = next_offset_;
            }
            return s;
        }

        // Загрузить объект в регистр
        operand load_to_reg(const std::string& name, size_t line) {
            auto it = slots_.find(name);
            if (it == slots_.end()) return operand::imm(0, ast::type{});
            auto& s = it->second;

            if (s.kind_ == slot_kind::REGISTER) return operand::reg(s.reg_name_, s.type_);

            // NEW: массив → возвращаем АДРЕС, а не содержимое
            if (s.type_.kind_ == ast::type_kind::ARRAY) {
                std::string tmp = new_vreg();
                lir_instruction& i = emit(lir_op::LEA, line);
                if (s.kind_ == slot_kind::STACK)
                    i.operands_ = { operand::reg(tmp, s.type_), operand::stack(s.offset_, s.type_) };
                else
                    i.operands_ = { operand::reg(tmp, s.type_), operand::global(s.hir_name_, s.type_) };
                return operand::reg(tmp, s.type_);
            }

            std::string tmp = new_vreg();
            lir_instruction& i = emit(lir_op::LD, line); // если в стеке или глобале, то сгенерить ld и вернуть новый регистр
            if (s.kind_ == slot_kind::STACK) {
                i.operands_ = { operand::reg(tmp, s.type_), operand::stack(s.offset_, s.type_) };
            } else {
                i.operands_ = { operand::reg(tmp, s.type_), operand::global(s.hir_name_, s.type_) };
            }
            return operand::reg(tmp, s.type_);
        }

        // Сохранить значение в объект
        void store_to(const std::string& name, const operand& src, size_t line) {
            auto it = slots_.find(name);
            if (it == slots_.end())
                return;
            auto& s = it->second;

            if (s.kind_ == slot_kind::REGISTER) {
                lir_instruction& i = emit(lir_op::MOV, line);
                i.operands_ = { operand::reg(s.reg_name_, s.type_), src };
            } else if (s.kind_ == slot_kind::STACK) {
                lir_instruction& i = emit(lir_op::ST, line);
                i.operands_ = { operand::stack(s.offset_, s.type_), src };
            } else {
                lir_instruction& i = emit(lir_op::ST, line);
                i.operands_ = { operand::global(s.hir_name_, s.type_), src };
            }
        }


        void lower_function(const ir::function& f, lir_function& lf) {
            lf.name_ = f.name_;
            lf.return_type_ = f.return_type_;
            fn_ = &lf;
            vreg_counter_ = 0;
            next_offset_ = 0;
            frame_used_ = 0;
            slots_.clear();

            collect_address_taken(f);

            // Параметры в стеке по положительным смещениям, если не помещаются в регистр
            int arg_off = 16;
            std::vector<std::pair<std::string,int>> preload;
            for (const auto& p : f.params_) {
                frame_slot s;
                s.hir_name_ = p->name_;
                s.type_ = p->type_;

                bool addr = address_taken_.count(p->name_) > 0;
                bool agg  = p->type_.kind_ == ast::type_kind::ARRAY;

                if (!addr && !agg) {
                    s.kind_ = slot_kind::REGISTER;
                    if (f.name_ == "main" && p->name_ == "argc") {
                        s.reg_name_ = "%r0";
                        new_vreg();
                    } else if (f.name_ == "main" && p->name_ == "argv") {
                        s.reg_name_ = "%r1";
                        new_vreg();
                    } else {
                        s.reg_name_ = new_vreg();
                        preload.emplace_back(p->name_, arg_off);
                    }
                } else {
                    s.kind_ = slot_kind::STACK;
                    s.offset_ = arg_off;
                }
                slots_[p->name_] = s;
                arg_off += 8;
            }

            // Локальные - создаем слоты
            for (const auto& l : f.locals_) {
                if (l->kind_ == ir::obj_kind::PARAM)
                    continue;
                if (slots_.count(l->name_))
                    continue;
                slots_[l->name_] = make_slot(*l);
            }


            lf.frame_size_ = align8(frame_used_);
            emit(lir_op::ENTER, 0).operands_ = { operand::imm(lf.frame_size_, ast::type{}) };

            // Прелоад параметров в регистры
            for (auto& [name, off] : preload) {
                auto& s = slots_[name];
                lir_instruction& i = emit(lir_op::LD, 0);
                i.operands_ = { operand::reg(s.reg_name_, s.type_),
                                operand::stack(off, s.type_) };
            }

            // Тело
            for (const auto& op : f.operations_)
                lower_op(*op); // понижаем операции

            // Eсли последняя операция не RET
            if (f.operations_.empty() || f.operations_.back()->code_ != ir::op_code::RET) {
                emit(lir_op::LEAVE, 0);
                emit(lir_op::RET, 0);
            }

            for (auto& [name, s] : slots_)
                lf.frame_.push_back(s);
        }

        void lower_op(const ir::operation& op) {
            switch (op.code_) {
                case ir::op_code::CONST: { // тут приводим все к одному типу long long
                    long long v = op.const_value_.int_value_;
                    if (op.const_value_.type_.kind_ == ast::type_kind::BOOL)
                        v = op.const_value_.bool_value_ ? 1 : 0;
                    if (op.const_value_.type_.kind_ == ast::type_kind::CHAR)
                        v = (unsigned char)op.const_value_.char_value_;
                    store_to(op.result_->name_, operand::imm(v, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::ASSIGN: {
                    auto src = load_to_reg(op.args_[1]->name_, op.line_);
                    store_to(op.args_[0]->name_, src, op.line_);
                    break;
                }

                case ir::op_code::ADD:
                case ir::op_code::SUB:
                case ir::op_code::MUL:
                case ir::op_code::DIV:
                case ir::op_code::AND:
                case ir::op_code::OR:
                case ir::op_code::XOR: {
                    auto a = load_to_reg(op.args_[0]->name_, op.line_);
                    auto b = load_to_reg(op.args_[1]->name_, op.line_);
                    std::string dst = new_vreg();
                    lir_instruction& i = emit(map_bin(op.code_), op.line_);
                    i.operands_ = { operand::reg(dst, op.result_->type_), a, b };
                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::NEG:
                case ir::op_code::NOT: {
                    auto a = load_to_reg(op.args_[0]->name_, op.line_);
                    std::string dst = new_vreg();
                    lir_instruction& i = emit(op.code_ == ir::op_code::NEG ? lir_op::NEG : lir_op::NOT, op.line_);
                    i.operands_ = { operand::reg(dst, op.result_->type_), a };
                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::LOAD: {
                    auto ptr = load_to_reg(op.args_[0]->name_, op.line_);
                    std::string dst = new_vreg();
                    lir_instruction& i = emit(lir_op::LD, op.line_);
                    i.operands_ = { operand::reg(dst, op.result_->type_), ptr };
                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::STORE: {
                    auto ptr = load_to_reg(op.args_[0]->name_, op.line_);
                    auto val = load_to_reg(op.args_[1]->name_, op.line_);
                    lir_instruction& i = emit(lir_op::ST, op.line_);
                    i.operands_ = { ptr, val };
                    break;
                }

                case ir::op_code::ADDR: {
                    auto it = slots_.find(op.args_[0]->name_);
                    if (it == slots_.end()) break;
                    auto& s = it->second;
                    std::string dst = new_vreg();
                    lir_instruction& i = emit(lir_op::LEA, op.line_);
                    if (s.kind_ == slot_kind::STACK) {
                        i.operands_ = { operand::reg(dst, op.result_->type_),
                                        operand::stack(s.offset_, s.type_) };
                    } else {
                        i.operands_ = { operand::reg(dst, op.result_->type_),
                                        operand::global(s.hir_name_, s.type_) };
                    }
                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::ELEM_PTR: {
                    auto arr = load_to_reg(op.args_[0]->name_, op.line_);
                    auto idx = load_to_reg(op.args_[1]->name_, op.line_);

                    int elem_size = op.result_->type_.base_ ? type_size(*op.result_->type_.base_) : 1;

                    std::string scaled = new_vreg();
                    lir_instruction& m = emit(lir_op::MUL, op.line_);
                    m.operands_ = { operand::reg(scaled, idx.type_), idx,
                                    operand::imm(elem_size, idx.type_) };

                    std::string dst = new_vreg();
                    lir_instruction& a = emit(lir_op::ADD, op.line_);
                    a.operands_ = { operand::reg(dst, op.result_->type_), arr,
                                    operand::reg(scaled, idx.type_) };

                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }

                case ir::op_code::CAST: {
                    auto src = load_to_reg(op.args_[0]->name_, op.line_);
                    store_to(op.result_->name_, src, op.line_);
                    break;
                }

                case ir::op_code::CALL: {
                    // Кладем аргументы на стек
                    for (auto& a : op.varargs_) {
                        auto v = load_to_reg(a->name_, op.line_);
                        lir_instruction& p = emit(lir_op::PUSH, op.line_);
                        p.operands_ = { v };
                    }
                    lir_instruction& c = emit(lir_op::CALL, op.line_);
                    c.operands_ = { operand::label(op.callee_name_) };

                    // Убираем аргументы (убираем в обратном порядке)
                    for (size_t k = 0; k < op.varargs_.size(); ++k) {
                        emit(lir_op::POP, op.line_);
                    }

                    // Возврат в %rax
                    if (op.result_) {
                        std::string tmp = new_vreg();
                        lir_instruction& mv = emit(lir_op::MOV, op.line_);
                        mv.operands_ = { operand::reg(tmp, op.result_->type_),
                                         operand::reg("%rax", op.result_->type_) };
                        store_to(op.result_->name_, operand::reg(tmp, op.result_->type_), op.line_);
                    }
                    break;
                }

                case ir::op_code::LABEL: {
                    lir_instruction& i = emit(lir_op::LABEL, op.line_);
                    i.operands_ = { operand::label(op.label_) };
                    break;
                }

                case ir::op_code::JUMP: {
                    lir_instruction& i = emit(lir_op::JMP, op.line_);
                    i.operands_ = { operand::label(op.label_) };
                    break;
                }

                case ir::op_code::BRANCH: {
                    auto cond = load_to_reg(op.args_[0]->name_, op.line_);

                    lir_instruction& jz = emit(lir_op::JZ, op.line_);
                    jz.operands_ = { cond, operand::label(op.false_label_) }; // два операнда

                    lir_instruction& jmp = emit(lir_op::JMP, op.line_);
                    jmp.operands_ = { operand::label(op.true_label_) };
                    break;
                }

                case ir::op_code::RET: {
                    if (!op.args_.empty()) {
                        auto v = load_to_reg(op.args_[0]->name_, op.line_);
                        lir_instruction& mv = emit(lir_op::MOV, op.line_);
                        mv.operands_ = { operand::reg("%rax", op.args_[0]->type_), v }; // результат ret записывается в %rax
                    }
                    emit(lir_op::LEAVE, op.line_);
                    emit(lir_op::RET, op.line_);
                    break;
                }
                case ir::op_code::EQ:
                case ir::op_code::NE:
                case ir::op_code::LT:
                case ir::op_code::LE:
                case ir::op_code::GT:
                case ir::op_code::GE: {
                    auto a = load_to_reg(op.args_[0]->name_, op.line_);
                    auto b = load_to_reg(op.args_[1]->name_, op.line_);

                    lir_op setcc;
                    switch (op.code_) {
                        case ir::op_code::EQ: setcc = lir_op::SET_EQ; break;
                        case ir::op_code::NE: setcc = lir_op::SET_NE; break;
                        case ir::op_code::LT: setcc = lir_op::SET_LT; break;
                        case ir::op_code::LE: setcc = lir_op::SET_LE; break;
                        case ir::op_code::GT: setcc = lir_op::SET_GT; break;
                        case ir::op_code::GE: setcc = lir_op::SET_GE; break;
                        default: setcc = lir_op::SET_EQ; break;
                    }

                    std::string dst = new_vreg();
                    lir_instruction& s = emit(setcc, op.line_);
                    s.operands_ = { operand::reg(dst, op.result_->type_), a, b }; // три операнда
                    store_to(op.result_->name_, operand::reg(dst, op.result_->type_), op.line_);
                    break;
                }
                default:
                    break;
            }
        }
        static lir_op map_bin(ir::op_code c) {
            switch (c) {
                case ir::op_code::ADD: return lir_op::ADD;
                case ir::op_code::SUB: return lir_op::SUB;
                case ir::op_code::MUL: return lir_op::MUL;
                case ir::op_code::DIV: return lir_op::DIV;
                case ir::op_code::AND: return lir_op::AND;
                case ir::op_code::OR:  return lir_op::OR;
                case ir::op_code::XOR: return lir_op::XOR;
                default: return lir_op::ADD;
            }
        }

    };
}
