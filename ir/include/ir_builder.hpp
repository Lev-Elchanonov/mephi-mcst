#pragma once
#include "ir_structs.hpp"
#include <unordered_map>
#include <string>

namespace ir {
    template <typename ast_prog, typename ast_func, typename ast_stmt, typename ast_expr>
    class builder {
        using program_ptr = std::shared_ptr<ast_prog>;
        using function_ptr = std::shared_ptr<ast_func>;
        using stmt_ptr = std::shared_ptr<ast_stmt>;
        using expr_ptr = std::shared_ptr<ast_expr>;

    private:
        translation_unit* unit_ = nullptr;
        function* func_ = nullptr;          // текущая функция, котороая сейчас строиться
        std::unordered_map<std::string, object_ptr> var_map_; // таблица переменных
        size_t temp_counter_ = 0;
        size_t label_counter_ = 0;
        std::string break_label_;
        std::string continue_label_;

    public:
        void build(const program_ptr& prog, translation_unit& unit) {
            unit_ = &unit;

            for (auto f : prog->functions_) {
                auto obj = std::make_shared<object>();
                obj->name_ = f->name_;
                obj->type_ = f->return_type_;
                obj->kind_ = obj_kind::GLOBAL;
                obj->is_function_ = true;
                unit.globals_.push_back(obj);
            }

            for (const auto& f : prog->functions_) {
                build_function(*f, unit);
            }
        }

    private:
        object_ptr new_temp(const ast::type& t) { // создание нового temp объекта
            auto o = std::make_shared<object>();
            o->name_ = "t" + std::to_string(temp_counter_++); // имя будет t0, t1, t2...
            o->type_ = t;
            o->kind_ = obj_kind::TEMP;
            func_->locals_.push_back(o); // пушим в func->locals_
            return o;
        }

        std::string new_label(const std::string& prefix) { // имена меток
            return prefix + std::to_string(label_counter_++);
        }

        operation_ptr emit(op_code code, size_t line) { // создание новой операции
            auto op = std::make_shared<operation>();
            op->code_ = code;
            op->line_ = line;
            func_->operations_.push_back(op);
            return op;
        }

        void build_function(const ast_func& func, translation_unit& unit) {
            var_map_.clear();
            temp_counter_ = 0;
            label_counter_ = 0;

            // заполняем данные о функции
            auto fn = std::make_unique<function>();
            fn->name_ = func.name_;
            fn->return_type_ = func.return_type_;
            func_ = fn.get();

            for (const auto& param : func.params_) {
                auto obj = std::make_shared<object>();
                obj->name_ = param.name_;
                obj->type_ = param.type_;
                obj->kind_ = obj_kind::PARAM;
                func_->params_.push_back(obj);
                func_->locals_.push_back(obj);
                var_map_[param.name_] = obj;
            }

            for (const auto& stmt : func.body_) {
                build_stmt(stmt);
            }
            if (func_->operations_.empty() || func_->operations_.back()->code_ != op_code::RET) {
                emit(op_code::RET, 0);
            }

            unit.functions_.push_back(std::move(fn));
            func_ = nullptr;
        }

        // дальше идут инструкции
        void build_stmt(const stmt_ptr& stmt) {
            if (!stmt)
                return;
            switch(stmt->kind_) {
                case ast::stmt_kind::VAR_DECL:  build_var_decl(*stmt); break;
                case ast::stmt_kind::ASSIGN:    build_assign(*stmt); break;
                case ast::stmt_kind::ASSIGN_OP: build_assign_op(*stmt); break;
                case ast::stmt_kind::IF:        build_if(*stmt); break;
                case ast::stmt_kind::WHILE:     build_while(*stmt); break;
                case ast::stmt_kind::RETURN:    build_return(*stmt); break;
                case ast::stmt_kind::EXPR_STMT: if (stmt->expr_) build_expr(stmt->expr_); break;
                case ast::stmt_kind::BLOCK:     for (auto& st : stmt->body_) build_stmt(st); break;
                case ast::stmt_kind::BREAK:     build_break(*stmt); break;
                case ast::stmt_kind::CONTINUE:  build_continue(*stmt); break;
            }
        }

        // построение stmt объявления переменной
        void build_var_decl(const ast_stmt& s) {
            auto obj = std::make_shared<object>();
            obj->name_ = s.name_;
            obj->type_ = s.var_type_;
            obj->kind_ = obj_kind::LOCAl;
            func_->locals_.push_back(obj);
            var_map_[s.name_] = obj;

            // если есть начальное значение
            if (s.init_value_) {
                auto src = build_expr(s.init_value_);
                if (src) {
                    auto op = emit(op_code::ASSIGN, s.line_);
                    op->args_ = { obj, src };
                }
            }
        }

        // построение присваивания
        void build_assign(const ast_stmt& s) {
            if (!s.target_)
                return;

            // если цель - это ссылка на переменную
            if (s.target_->kind_ == ast::expr_kind::VAR_REF) {
                auto it = var_map_.find(s.target_->name_); // ищем в мапе
                if (it != var_map_.end()) {
                    auto src = build_expr(s.value_); // строим expr для value
                    if (src) {
                        auto op = emit(op_code::ASSIGN, s.line_); // создаем операцию
                        op->args_ = { it->second, src };
                    }
                }
            }  else if (s.target_->kind_ == ast::expr_kind::INDEX) { // если это индекс массива
                auto ptr = build_elem_ptr(s.target_); // указатель на элемент
                auto val = build_expr(s.value_); // построение выражения для value
                if (ptr && val) {
                    auto op = emit(op_code::STORE, s.line_); // хранение
                    op->args_ = { ptr, val };
                }
            }
        }

        // это для -=, += и тд
        void build_assign_op(const ast_stmt& s) {
            if (!s.target_ || s.target_->kind_ != ast::expr_kind::VAR_REF) return;
            auto it = var_map_.find(s.target_->name_);
            if (it == var_map_.end()) return;

            auto dst = it->second;
            auto rhs = build_expr(s.value_);
            if (!rhs) return;

            auto tmp = new_temp(dst->type_);
            auto op = emit(bin_op_code(s.assign_op_), s.line_);
            op->args_ = { dst, rhs };
            op->result_ = tmp;

            auto op2 = emit(op_code::ASSIGN, s.line_);
            op2->args_ = { dst, tmp };
        }

        // построение if
        void build_if(const ast_stmt& s) {
            auto condition = build_expr(s.condition_);
            if (!condition) return;

            // создаем новые метки
            auto then_lbl = new_label("if_then_");
            auto else_lbl = new_label("if_else_");
            auto end_lbl = new_label("if_end_");

            auto br = emit(op_code::BRANCH, s.line_);
            br->args_ = { condition };
            br->true_label_ = then_lbl;
            br->false_label_ = s.else_body_.empty() ? end_lbl : else_lbl;

            emit(op_code::LABEL, s.line_)->label_ = then_lbl;
            // проходимся по всей области видимости
            for (auto& st : s.then_body_)
                build_stmt(st);

            // если else не пустое
            if (!s.else_body_.empty()) {
                emit(op_code::JUMP, s.line_)->label_ = end_lbl; // прыгаем на end_lbl
                emit(op_code::LABEL, s.line_)->label_ = else_lbl;
                for (auto& st : s.else_body_)
                    build_stmt(st);
            }
            emit(op_code::LABEL, s.line_)->label_ = end_lbl;
        }

        void build_while(const ast_stmt& s) {
            auto start_lbl = new_label("while_");
            auto body_lbl = new_label("while_body_");
            auto end_lbl = new_label("while_end_");

            emit(op_code::LABEL, s.line_)->label_ = start_lbl;

            auto cond = build_expr(s.condition_);
            if (!cond) return;

            auto br = emit(op_code::BRANCH, s.line_);
            br->args_ = { cond };
            br->true_label_ = body_lbl;
            br->false_label_ = end_lbl;

            emit(op_code::LABEL, s.line_)->label_ = body_lbl;

            auto old_break = break_label_;
            auto old_continue = continue_label_;
            break_label_ = end_lbl;
            continue_label_ = start_lbl;

            for (const auto& st : s.then_body_)
                build_stmt(st);

            break_label_ = old_break;
            continue_label_ = old_continue;

            emit(op_code::JUMP, s.line_)->label_ = start_lbl;
            emit(op_code::LABEL, s.line_)->label_ = end_lbl;
        }

        void build_return(const ast_stmt& s) {
            object_ptr v = nullptr;
            if (s.return_value_) {
                v = build_expr(s.return_value_);   // сначала выражение
            }
            auto op = emit(op_code::RET, s.line_); // потом RET
            if (v) op->args_ = { v };
        }

        void build_break(const ast_stmt& s) {
            if (!break_label_.empty()) {
                emit(op_code::JUMP, s.line_)->label_ = break_label_; // метка конца текущего цикла
            }
        }

        void build_continue(const ast_stmt& s) {
            if (!continue_label_.empty()) {
                emit(op_code::JUMP, s.line_)->label_ = continue_label_;
            }
        }

        // выражения
        object_ptr build_expr(const expr_ptr& expr) {
            if (!expr) return nullptr;

            switch (expr->kind_) {
                case ast::expr_kind::INT_LIT:
                case ast::expr_kind::BOOL_LIT:
                case ast::expr_kind::CHAR_LIT:
                case ast::expr_kind::STRING_LIT: {
                    auto t = new_temp(expr->type_);
                    auto op = emit(op_code::CONST, expr->line_);
                    op->result_ = t;
                    op->const_value_.type_ = expr->type_;
                    op->const_value_.int_value_ = expr->int_value_;
                    op->const_value_.bool_value_ = expr->bool_value_;
                    op->const_value_.char_value_ = expr->char_value_;
                    op->const_value_.string_value_ = expr->string_value_;
                    return t;
                }
                case ast::expr_kind::VAR_REF: {
                    auto it = var_map_.find(expr->name_);
                    if (it != var_map_.end()) return it->second;
                    return nullptr;
                }
                case ast::expr_kind::BIN_OP: {
                    auto lhs = build_expr(expr->lhs_);
                    auto rhs = build_expr(expr->rhs_);
                    if (!lhs || !rhs) return nullptr;
                    auto t = new_temp(expr->type_);
                    auto op = emit(bin_op_code(expr->bin_op_), expr->line_);
                    op->args_ = { lhs, rhs };
                    op->result_ = t;
                    return t;
                }
                case ast::expr_kind::UN_OP: {
                    auto operand = build_expr(expr->lhs_);
                    if (!operand) return nullptr;
                    auto t = new_temp(expr->type_);
                    auto op = emit(un_op_code(expr->un_op_), expr->line_);
                    op->args_ = { operand };
                    op->result_ = t;
                    return t;
                }
                case ast::expr_kind::CALL: {
                    std::vector<object_ptr> args;
                    for (auto& a : expr->args_) {
                        auto obj = build_expr(a);
                        if (obj) args.push_back(obj);
                    }
                    auto t = new_temp(expr->type_);
                    auto op = emit(op_code::CALL, expr->line_);
                    op->result_ = t;
                    op->callee_name_ = expr->name_;
                    op->varargs_ = args;
                    return t;
                }
                case ast::expr_kind::INDEX: {
                    auto ptr = build_elem_ptr(expr);
                    if (!ptr)
                        return nullptr;
                    auto t = new_temp(expr->type_);
                    auto op = emit(op_code::LOAD, expr->line_);
                    op->args_ = { ptr };
                    op->result_ = t;
                    return t;
                }
            }
            return nullptr;
        }
        object_ptr build_elem_ptr(const expr_ptr& expr) {
            auto arr = build_expr(expr->lhs_);
            auto idx = build_expr(expr->rhs_);
            if (!arr || !idx) return nullptr;

            auto ptr_type = ast::type{
                ast::type_kind::POINTER,
                std::make_shared<ast::type>(expr->type_),
                0
            };
            auto t = new_temp(ptr_type);
            auto op = emit(op_code::ELEM_PTR, expr->line_);
            op->args_ = { arr, idx };
            op->result_ = t;
            return t;
        }

        static op_code bin_op_code(ast::binary_op op) {
            switch (op) {
                case ast::binary_op::ADD:     return op_code::ADD;
                case ast::binary_op::SUB:     return op_code::SUB;
                case ast::binary_op::MUL:     return op_code::MUL;
                case ast::binary_op::DIV:     return op_code::DIV;
                case ast::binary_op::BIN_AND: return op_code::AND;
                case ast::binary_op::BIN_OR:  return op_code::OR;
                case ast::binary_op::EQ:      return op_code::EQ;
                case ast::binary_op::NE:      return op_code::NE;
                case ast::binary_op::LT:      return op_code::LT;
                case ast::binary_op::LE:      return op_code::LE;
                case ast::binary_op::GT:      return op_code::GT;
                case ast::binary_op::GE:      return op_code::GE;
                case ast::binary_op::LOG_AND: return op_code::LOG_AND;
                case ast::binary_op::LOG_OR:  return op_code::LOG_OR;
            }
            return op_code::ADD;
        }
        static op_code un_op_code(ast::unary_op op) {
            switch (op) {
                case ast::unary_op::NEG:     return op_code::NEG;
                case ast::unary_op::LOG_NOT: return op_code::NOT;
            }
            return op_code::NEG;
        }

        using ast_builder = builder<ast::program, ast::func_decl, ast::stmt, ast::expr>;
    };


}
