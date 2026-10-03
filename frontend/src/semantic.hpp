#pragma once

#include <iostream>
#include <unordered_map>

#include "ast.hpp"
namespace sema {
    struct error {
        size_t line_;
        size_t column_;
        std::string msg_;
    };

    struct symbol {
        ast::type type_;
        bool is_const_ = false;
        size_t line_ = 0;
    };

    // класс для анализа областей видимости
    class scope {
    public:
        std::unordered_map<std::string, symbol> symbols_; // все символы в текущем скоупе
        scope* parent_ = nullptr; // ссылка на родительский скоуп

        symbol* find(const std::string& name) {
            for (scope* s = this; s; s = s->parent_) { // переходим по родителям пока не найдем нужный символ
                auto it = s->symbols_.find(name);
                if (it != s->symbols_.end())
                    return &it->second;
            }
            return nullptr;
        }

        symbol* find_local(const std::string& name) {
            auto it = symbols_.find(name);
            return it == symbols_.end() ? nullptr : &it->second;
        }

        bool add_symb(const std::string& name, const symbol& sym) {
            if (symbols_.contains(name))
                return false;
            symbols_[name] = sym;
            return true;
        }
    };


    class analyzer {
    private:
        std::vector<error> errors_;
        scope* current_ = nullptr;
        scope global_;

        struct func_info { // вспомогательная структура для функций
            ast::type return_type_;
            std::vector<ast::type> params_type_;
        };

        std::unordered_map<std::string, func_info> funcs_; // все функции
        std::vector<std::unique_ptr<scope>> scopes_;
        ast::type current_return_type_;

        void register_builtin_funcs() {
            func_info prints;
            prints.return_type_ = ast::type(ast::type_kind::VOID, nullptr, 0);
            prints.params_type_ = { ast::type{ast::type_kind::POINTER, std::make_shared<ast::type>(ast::type{ast::type_kind::CHAR, nullptr, 0}), 0} };
            funcs_["prints"] = prints;

            func_info printi;
            printi.return_type_ = ast::type{ast::type_kind::VOID, nullptr, 0};
            printi.params_type_ = { ast::type{ast::type_kind::I32, nullptr, 0} };
            funcs_["printi"] = printi;
        }

        void add_error(size_t line, size_t column, const std::string& msg) { // добавление новой ошибки в массив ошибок
            errors_.push_back({line, column, msg});
        }

        static std::string type_name(const ast::type& t) { // поиск типа
            switch (t.kind_) {
                case ast::type_kind::I32:     return "i32";
                case ast::type_kind::BOOL:    return "bool";
                case ast::type_kind::CHAR:    return "char";
                case ast::type_kind::VOID:    return "void";
                case ast::type_kind::POINTER: return "ptr";
                case ast::type_kind::ARRAY:   return "array";
            }
            return "?";
        }

        static bool is_numeric(const ast::type& t) {
            return t.kind_ == ast::type_kind::I32  ||
                   t.kind_ == ast::type_kind::CHAR ||
                   t.kind_ == ast::type_kind::BOOL;
        }


        static bool same_type(const ast::type& a, const ast::type& b) {
            if (a.kind_ != b.kind_)
                return false;
            if (a.kind_ == ast::type_kind::POINTER || a.kind_ == ast::type_kind::ARRAY) {
                if (!a.base_ || !b.base_)
                    return a.base_ == b.base_;
                return same_type(*a.base_, *b.base_);
            }
            return true;
        }


        void analyze_entry(const std::shared_ptr<ast::program>& program) { // точка входа анализтора
            for (const auto& f : program->functions_) { // сначала делаем сбор всех функций
                if (funcs_.contains(f->name_)) {
                    add_error(f->line_, f->column_, "function '" + f->name_ + "' already defined");
                    continue;
                }
                func_info info;
                info.return_type_ = f->return_type_;
                for (const auto& p : f->params_)
                    info.params_type_.push_back(p.type_);
                funcs_[f->name_] = info;
            }
            if (!funcs_.count("main")) { // проверка на наличие main. Он должен быть обязательно
                add_error(0, 0, "function 'main' not found");
            }

            for (const auto& f : program->functions_) { // проверка каждой функции
                analy_func(*f);
            }
        }
        void analy_func(const ast::func_decl& f) {
            scope scope_;
            scope_.parent_ = &global_;
            current_ = &scope_;

            current_return_type_ = f.return_type_;

            for (const auto& p : f.params_) {
                symbol sym;
                sym.type_ = p.type_;
                sym.line_ = f.line_;
                if (!scope_.add_symb(p.name_, sym)) {
                    add_error(f.line_, f.column_, "parameter '" + p.name_ + "' already declared");
                }
            }

            // Анализ тела функции
            for (const auto& st : f.body_) {
                analy_stmt(st);
            }

            // когда проанализировали всю функцию надо откатиться до глобальной области видимости, в которой лежат вызовы других функий
            current_ = &global_;
        }

        void analy_stmt(const ast::stmt_ptr& s) {
            if (!s)
                return;
            switch (s->kind_) {
                case ast::stmt_kind::VAR_DECL: { // если это объявление переменной
                    if (current_->find_local(s->name_)) {
                        add_error(s->line_, s->column_, "variable '" + s->name_ + "' already declared");
                        return;
                    }
                    symbol sym;
                    sym.type_ = s->var_type_;
                    sym.line_ = s->line_;
                    current_->add_symb(s->name_, sym);

                    if (s->init_value_) {
                        analy_expr(s->init_value_);
                        if (!can_assign(s->var_type_, s->init_value_->type_)) { // если присвоить нельзя
                            add_error(s->line_, s->column_,
                                  "cannot assign " + type_name(s->init_value_->type_) + " to " + type_name(s->var_type_) + " variable '" + s->name_ + "'");
                        }
                    }
                    break;
                }

                case ast::stmt_kind::ASSIGN:
                case ast::stmt_kind::ASSIGN_OP: {
                    if (s->target_)
                        analy_expr(s->target_);
                    if (s->value_)
                        analy_expr(s->value_);

                    if (s->target_ && s->target_->kind_ != ast::expr_kind::VAR_REF &&
                        s->target_->kind_ != ast::expr_kind::INDEX) {
                        add_error(s->line_, s->column_, "assignment target is not lvalue");
                        }
                    if (s->target_ && s->value_ &&
                        !can_assign(s->target_->type_, s->value_->type_)) {
                        add_error(s->line_, s->column_,
                              "type mismatch in assignment: " +
                              type_name(s->value_->type_) + " to " +
                              type_name(s->target_->type_));
                        }
                    break;
                }

                case ast::stmt_kind::IF: {
                    if (s->condition_) {
                        analy_expr(s->condition_);
                        if (s->condition_->type_.kind_ != ast::type_kind::BOOL) { // возвращаемый тип condition в if stmt обязан быть bool
                            add_error(s->line_, s->column_, "if condition must be bool");
                        }
                    }
                    push_scope(); // создаем новую область видимости на время
                    for (auto& st : s->then_body_)
                        analy_stmt(st);
                    pop_scope(); // попаем ее после выполнения then блока

                    push_scope(); // тут по аналогии
                    for (auto& st : s->else_body_)
                        analy_stmt(st);
                    pop_scope();
                    break;
                }

                case ast::stmt_kind::WHILE: {
                    if (s->condition_) {
                        analy_expr(s->condition_);
                        if (s->condition_->type_.kind_ != ast::type_kind::BOOL) {
                            add_error(s->line_, s->column_, "while condition must be bool");
                        }
                    }
                    push_scope();
                    for (auto& st : s->then_body_)
                        analy_stmt(st);
                    pop_scope();
                    break;
                }

                case ast::stmt_kind::RETURN: {
                    if (s->return_value_) {
                        analy_expr(s->return_value_);

                        if (!can_assign(current_return_type_, s->return_value_->type_)) {
                            add_error(s->line_, s->column_, "return type mismatch: expected " +
                                      type_name(current_return_type_) + ", got " + type_name(s->return_value_->type_));
                        }
                    } else if (current_return_type_.kind_ != ast::type_kind::VOID) {
                        add_error(s->line_, s->column_, "return type mismatch: expected " + type_name(current_return_type_) + ", got void");
                    }
                    break;
                }

                case ast::stmt_kind::EXPR_STMT: {
                    if (s->expr_)
                        analy_expr(s->expr_);
                    break;
                }

                case ast::stmt_kind::BLOCK: {
                    push_scope();
                    for (auto& st : s->body_)
                        analy_stmt(st);
                    pop_scope();
                    break;
                }

                case ast::stmt_kind::BREAK:
                case ast::stmt_kind::CONTINUE:
                    break;
            }
        }
        void analy_expr(const ast::expr_ptr& e) {
            if (!e)
                return;

            // сначала обработка детей
            analy_expr(e->lhs_);
            analy_expr(e->rhs_);
            for (auto& arg : e->args_) // выражение - вызов функции
                analy_expr(arg);

            switch (e->kind_) {
                case ast::expr_kind::INT_LIT:
                    e->type_ = ast::type{ast::type_kind::I32, nullptr, 0};
                    break;
                case ast::expr_kind::BOOL_LIT:
                    e->type_ = ast::type{ast::type_kind::BOOL, nullptr, 0};
                    break;
                case ast::expr_kind::CHAR_LIT:
                    e->type_ = ast::type{ast::type_kind::CHAR, nullptr, 0};
                    break;
                case ast::expr_kind::STRING_LIT:
                    e->type_ = ast::type{ast::type_kind::POINTER,
                                         std::make_shared<ast::type>(ast::type{ast::type_kind::CHAR, nullptr, 0}),
                                         0};
                    break;

                case ast::expr_kind::VAR_REF: {
                    symbol* sym = current_->find(e->name_);
                    if (!sym) {
                        add_error(e->line_, e->column_,"variable '" + e->name_ + "' not declared");
                        e->type_ = ast::type{ast::type_kind::VOID, nullptr, 0};
                    } else {
                        e->type_ = sym->type_;
                    }
                    break;
                }

                case ast::expr_kind::BIN_OP: {
                    auto lt = e->lhs_ ? e->lhs_->type_ : ast::type{};
                    auto rt = e->rhs_ ? e->rhs_->type_ : ast::type{};

                    switch (e->bin_op_) {
                        case ast::binary_op::ADD:
                        case ast::binary_op::SUB:
                        case ast::binary_op::MUL:
                        case ast::binary_op::DIV: {
                            if (!is_numeric(lt) || !is_numeric(rt)) {
                                add_error(e->line_, e->column_,"arithmetic requires numeric operands");
                            }
                            e->type_ = ast::type{ast::type_kind::I32, nullptr, 0};
                            break;
                        }
                        case ast::binary_op::BIN_AND:
                        case ast::binary_op::BIN_OR: {
                            if (!is_numeric(lt) || !is_numeric(rt)) {
                                add_error(e->line_, e->column_,"bitwise op requires numeric operands");
                            }
                            e->type_ = ast::type{ast::type_kind::I32, nullptr, 0};
                            break;
                        }
                        case ast::binary_op::EQ:
                        case ast::binary_op::NE: {
                            if (!same_type(lt, rt)) {
                                add_error(e->line_, e->column_,"comparison of different types");
                            }
                            e->type_ = ast::type{ast::type_kind::BOOL, nullptr, 0};
                            break;
                        }
                        case ast::binary_op::LT:
                        case ast::binary_op::LE:
                        case ast::binary_op::GT:
                        case ast::binary_op::GE: {
                            if (!is_numeric(lt) || !is_numeric(rt)) {
                                add_error(e->line_, e->column_,"comparison requires numeric operands");
                            }
                            e->type_ = ast::type{ast::type_kind::BOOL, nullptr, 0};
                            break;
                        }
                        case ast::binary_op::LOG_AND:
                        case ast::binary_op::LOG_OR: {
                            if (lt.kind_ != ast::type_kind::BOOL ||
                                rt.kind_ != ast::type_kind::BOOL) {
                                add_error(e->line_, e->column_,"logical op requires bool operands");
                                }
                            e->type_ = ast::type{ast::type_kind::BOOL, nullptr, 0};
                            break;
                        }
                    }
                    break;
                }

                case ast::expr_kind::UN_OP: {
                    auto t = e->lhs_ ? e->lhs_->type_ : ast::type{};
                    if (e->un_op_ == ast::unary_op::NEG) {
                        if (!is_numeric(t)) {
                            add_error(e->line_, e->column_, "unary - requires numeric");
                        }
                        e->type_ = ast::type{ast::type_kind::I32, nullptr, 0};
                    } else if (e->un_op_ == ast::unary_op::LOG_NOT) {
                        if (t.kind_ != ast::type_kind::BOOL) {
                            add_error(e->line_, e->column_, "! requires bool");
                        }
                        e->type_ = ast::type{ast::type_kind::BOOL, nullptr, 0};
                    }
                    break;
                }

                case ast::expr_kind::CALL: {
                    auto it = funcs_.find(e->name_);
                    if (it == funcs_.end()) {
                        add_error(e->line_, e->column_,"function '" + e->name_ + "' not declared");
                        e->type_ = ast::type{ast::type_kind::VOID, nullptr, 0};
                        break;
                    }
                    auto& info = it->second;
                    if (info.params_type_.size() != e->args_.size()) {
                        add_error(e->line_, e->column_, "function '" + e->name_ + "' expects " +
                            std::to_string(info.params_type_.size()) + " args, got " + std::to_string(e->args_.size()));
                    } else {
                        for (size_t i = 0; i < e->args_.size(); ++i) {
                            if (!can_assign(info.params_type_[i], e->args_[i]->type_)) {
                                add_error(e->line_, e->column_,
                                      "argument " + std::to_string(i + 1) + " of '" + e->name_ + "': cannot pass " +
                                      type_name(e->args_[i]->type_) + " as " + type_name(info.params_type_[i]));
                            }
                        }
                    }
                    e->type_ = info.return_type_;
                    break;
                }

                case ast::expr_kind::INDEX: {
                    if (e->lhs_) {
                        if (e->lhs_->type_.kind_ != ast::type_kind::ARRAY &&
                            e->lhs_->type_.kind_ != ast::type_kind::POINTER) {
                            add_error(e->line_, e->column_, "indexing non-array");
                            }
                        if (e->lhs_->type_.kind_ == ast::type_kind::ARRAY && e->lhs_->type_.base_) {
                            e->type_ = *e->lhs_->type_.base_;
                        } else if (e->lhs_->type_.kind_ == ast::type_kind::POINTER && e->lhs_->type_.base_) {
                            e->type_ = *e->lhs_->type_.base_;
                        } else {
                            e->type_ = ast::type{ast::type_kind::I32, nullptr, 0};
                        }
                    }
                    if (e->rhs_ && !is_numeric(e->rhs_->type_)) {
                        add_error(e->line_, e->column_, "index must be numeric");
                    }
                    break;
                }
            }
        }
        // функция проверяющая, можно ли присвоить один тип другому
        static bool can_assign(const ast::type& dst, const ast::type& src) {
            if (same_type(dst, src))
                return true;
            // все числовые типы - совместимы
            if (is_numeric(dst) && is_numeric(src)) return true;

            // если это указатели
            if (dst.kind_ == ast::type_kind::POINTER && src.kind_ == ast::type_kind::POINTER) {
                return same_type(*dst.base_, *src.base_);
            }
            return false;
        }

        // добавление нового скоупа
        void push_scope() {
            scope* s = new scope();
            s->parent_ = current_;
            current_ = s;
            scopes_.push_back(std::unique_ptr<scope>(s));
        }

        // удаление скоупа
        void pop_scope() {
            if (current_->parent_)
                current_ = current_->parent_;
        }
    public:
        bool analyze(const std::shared_ptr<ast::program>& program) {
            analyze_entry(program);
            for (const auto& e : errors_) {
                std::cerr << "SEMANTIC ERROR: " << e.msg_ << std::endl;
            }
            return errors_.empty();
        }
        analyzer() {
            current_ = &global_;
            register_builtin_funcs();
        }
        std::vector<error> get_errors() const { return errors_; }

    };
}