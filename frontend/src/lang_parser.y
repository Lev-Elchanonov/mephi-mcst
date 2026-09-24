%require "3.2"
%language "c++"
%defines "parser.tab.hpp"
%output "parser.tab.cpp"

// тут код, который будет в hpp файле parser.tab.hpp
// он тут нужен для подключения в ast.hpp

%code requires{
    #include <memory>
    #include <string>
    #include <vector>
    #include "ast.hpp"

    namespace yy { class lang_lexer; }
}

%code provides{
    extern yy::lang_lexer* THE_LEXER;
}
// тут код, который попадает в parser.tab.cpp
%code {
    #include <iostream>
    #include "lang_lexer.hpp"
    #include "ast.hpp"

    int syntax_errors = 0;

    std::shared_ptr<ast::program> program;

    void yy::Parser::error(const location_type& loc, const std::string& msg){
        std::cerr << "SYNTAX ERROR (line " << loc.begin.line<< "): " << msg << std::endl;
        syntax_errors++;
    }

    yy::lang_lexer* THE_LEXER = nullptr;
    yy::Parser::location_type THE_LOC;

    static yy::Parser::symbol_type yylex() {
        return THE_LEXER->yylex(&THE_LOC);
    }


}

// Тут настройки парсера
%define api.namespace { yy }
%define api.parser.class { Parser }
%define api.token.constructor
%define api.value.type variant
%define parse.error verbose     // подробные сообщения об ошибках


%locations // для отслеживания позиции

// передача лексера в парсер
//%lex-param  { yy::lang_lexer& lexer }
%parse-param { yy::lang_lexer& lexer }

// Токены
%token EOF 0

%token I32 BOOL CHAR VOID PTR
%token IF ELIF ELSE WHILE RETURN BREAK CONTINUE

%token <long long> INT_LIT
%token <bool> BOOL_LIT
%token <char> CHAR_LIT
%token <std::string> STRING_LIT
%token <std::string> IDENTIFIER

%token EQ NE LE GE LOG_AND LOG_OR
%token PLUS_EQ MINUS_EQ STAR_EQ SLASH_EQ
%token PLUS MINUS STAR SLASH
%token AMP PIPE BANG
%token LT GT ASSIGN
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token SEMICOLON COMMA

// все типы нетерминалов
%type <ast::type> type_spec base_type
// type_spec - тип в объявлении (i32, ptr$, i32[10], char*)
// base_type - базовый тип без модификаторов: i32, bool, char, void

%type <ast::expr_ptr> expr primary_expr unary_expr binary_expr
// expr - любое выражение
// primary_expr - любое выражение без операторов (42, true, 'a', )
// unary_expr - выражение с унарным оператором (-x, !value...)
// binary_expr - выражение с бинарным оператором (a + b...)

%type <ast::stmt_ptr> stmt simple_stmt compound_stmt var_decl
// stmt - любая инструкция
// simple_stmt - простая инструкция (x = 5, return 0, break)
// compound_stmt - составная (if, while, блок { })
// var_decl - объявление переменной (i32 x = 5)

%type <std::vector<ast::stmt_ptr>> stmt_list block opt_elif_else
// stmt_list - последовательность идущих инструкций
// block - { stmt_list }
// opt_elif_else - тоже последовательность инструкций

%type <std::vector<ast::expr_ptr>> opt_arg_list arg_list
// arg_list - непустой список аргументов (a, b, c, d...)
// opt_arg_list - список аргументов или пусто (это надо для CALL функций foo())

%type <ast::param> param
// param - структура с type_и name_

%type <std::vector<ast::param>> param_list opt_param_list
// param_list - непустой список параметров (i32 a, bool b)
// opt_param_list - список или пусто (для foo() без параметров)

%type <std::shared_ptr<ast::func_decl>> func_def
// func_def - одно объявление функции целиком


// задание приоритетов операторов
// тут задание идет от низкого к высокому
// логические - самые слабые, потом сравнения, потом арифметика, потом унарные, потом скобки и вызовы
%left LOG_OR
%left LOG_AND
%left AMP PIPE
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left STAR SLASH
%right UMINUS LOG_NOT
%left LPAREN RPAREN LBRACKET RBRACKET

%start program

%%

program:
    func_def_list
    {

    }
;

// список объявлений функций
func_def_list:
    %empty
    | func_def_list func_def
    {
        program->functions_.push_back($2);
    }
    | func_def_list error
    {
        // при синтаксической ошибке восстанавливаем автомат
        yyerrok;
        yyclearin;
    }
;

// объявление одной функции
func_def:
    type_spec IDENTIFIER LPAREN opt_param_list RPAREN block
    {
        auto func = std::make_shared<ast::func_decl>();
        func->return_type_ = $1;
        func->name_ = $2;
        func->params_ = $4;
        func->body_ = $6;
        func->line_ = @2.begin.line;
        func->column_ = @2.begin.column;
        $$ = func;
    }
;

// опциональный список параметров
opt_param_list:
    %empty              { $$ = {}; }
    | param_list        { $$ = $1; }
;

param_list:
    param
    {
        $$ = { $1 };
    }
    | param_list COMMA param
    {
        $1.push_back($3);
        $$ = $1;
    }
;

param:
    type_spec IDENTIFIER
    {
        $$ = ast::param{$1, $2};
    }
;

type_spec:
    base_type
    {
        $$ = $1;
    }
    | PTR type_spec
    {
        // это правило ptr$ <type>
        $$ = ast::type{ast::type_kind::POINTER, std::make_shared<ast::type>($2), 0};
    }
    | type_spec STAR
    {
        // <type>* - постфиксный указатель (c-style)
        $$ = ast::type{ast::type_kind::POINTER, std::make_shared<ast::type>($1), 0};
    }
    | type_spec LBRACKET INT_LIT RBRACKET
    {
        // <type>[N] - массив фиксированного размера
         $$ = ast::type{ast::type_kind::ARRAY, std::make_shared<ast::type>($1), (size_t)$3};
    }
    | type_spec LBRACKET RBRACKET
    {
        // <type>[] — массив неизвестного размера
        $$ = ast::type{ast::type_kind::ARRAY, std::make_shared<ast::type>($1), 0};
    }
;

base_type:
    I32     { $$ = ast::type{ast::type_kind::I32, nullptr, 0}; }
    | BOOL  { $$ = ast::type{ast::type_kind::BOOL, nullptr, 0}; }
    | CHAR  { $$ = ast::type{ast::type_kind::CHAR, nullptr, 0}; }
    | VOID  { $$ = ast::type{ast::type_kind::VOID, nullptr, 0}; }
;

// правила для блока и списка инструкций
block:
    LBRACE stmt_list RBRACE
    {
        $$ = $2;
    }
;

stmt_list:
    %empty
    {
        $$ = {};
    }
    | stmt_list stmt
    {
        $1.push_back($2);
        $$ = $1;
    }
;

stmt:
    simple_stmt             { $$ = $1; }
    | compound_stmt         { $$ = $1; }
;

// простые конструкции
simple_stmt:
    var_decl SEMICOLON  { $$ = $1; }

    // выражение как инструкция: foo()
    | expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::EXPR_STMT;
        st->expr_ = $1;
        st->line_ = @1.begin.line;
        $$ = st;
    }

    // return expr
    | RETURN expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::RETURN;
        st->return_value_ = $2;
        st->line_ = @1.begin.line;
        $$ = st;
    }

    // обычный return
    | RETURN SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::RETURN;
        st->line_ = @1.begin.line;
        $$ = st;
    }

    // break
    | BREAK SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::BREAK;
        $$ = st;
    }

    //continue
    | CONTINUE SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::CONTINUE;
        $$ = st;
    }

    // присваивание: a = expr;
    | IDENTIFIER ASSIGN expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN;
        st->line_ = @2.begin.line;

        auto target = std::make_shared<ast::expr>();
        target->kind_ = ast::expr_kind::VAR_REF;
        target->name_ = $1;
        target->line_ = @1.begin.line;
        target->column_ = @1.begin.column;

        st->target_ = target;
        st->value_ = $3;
        $$ = st;
    }

    // +=
    | IDENTIFIER PLUS_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::ADD;
        st->line_ = @2.begin.line;

        auto target = std::make_shared<ast::expr>();
        target->kind_ = ast::expr_kind::VAR_REF;
        target->name_ = $1;
        target->line_ = @1.begin.line;

        st->target_ = target;
        st->value_ = $3;
        $$ = st;
    }

    // *=
    | IDENTIFIER STAR_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::MUL;
        st->line_ = @2.begin.line;

        auto target = std::make_shared<ast::expr>();
        target->kind_ = ast::expr_kind::VAR_REF;
        target->name_ = $1;
        target->line_ = @1.begin.line;

        st->target_ = target;
        st->value_ = $3;
        $$ = st;
    }

    // /=
    | IDENTIFIER SLASH_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::DIV;
        st->line_ = @2.begin.line;

        auto target = std::make_shared<ast::expr>();
        target->kind_ = ast::expr_kind::VAR_REF;
        target->name_ = $1;
        target->line_ = @1.begin.line;

        st->target_ = target;
        st->value_ = $3;
        $$ = st;
    }
;

// объявление переменной
var_decl:
    type_spec IDENTIFIER // (i32 x)
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::VAR_DECL;
        st->var_type_ = $1;
        st->name_ = $2;
        st->line_ = @2.begin.line;
        st->column_ = @2.begin.column;
        $$ = st;
    }
    | type_spec IDENTIFIER ASSIGN expr // (i32 x = 2)
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::VAR_DECL;
        st->var_type_ = $1;
        st->name_ = $2;
        st->init_value_ = $4;
        st->line_ = @2.begin.line;
        st->column_ = @2.begin.column;
        $$ = st;
    }
;

// составные конструкции
compound_stmt:
    IF LPAREN expr RPAREN block // if (cond) { ... }
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::IF;
        st->condition_ = $3;
        st->then_body_ = $5;
        st->line_ = @1.begin.line;
        $$ = st;
    }
    | IF LPAREN expr RPAREN block ELSE block // if (cond) { ... } else { ... }
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::IF;
        st->condition_ = $3;
        st->then_body_ = $5;
        st->else_body_ = $7;
        st->line_ = @1.begin.line;
        $$ = st;
    }
    | WHILE LPAREN expr RPAREN block // while (cond) { ... }
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::WHILE;
        st->condition_ = $3;
        st->then_body_ = $5;
        st->line_ = @1.begin.line;
        $$ = st;
    }
    | block // просто блок { ... } как инструкция
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::BLOCK;
        st->body_ = $1;
        $$ = st;
    }
;

// Хвост для elif: пусто, else { ... }, или ещё один elif ...
opt_elif_else:
    %empty                      { $$ = {}; }
    | ELSE block                { $$ = $2; }
    | ELIF LPAREN expr RPAREN block opt_elif_else
    {
        auto inner = std::make_shared<ast::stmt>();
        inner->kind_ = ast::stmt_kind::IF;
        inner->condition_ = $3;
        inner->then_body_ = $5;
        inner->else_body_ = $6;
        $$ = { inner };
    }
;

// выражения
expr:
    binary_expr
;

binary_expr:
unary_expr
    | binary_expr PLUS binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::ADD;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr MINUS binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::SUB;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr STAR binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::MUL;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr SLASH binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::DIV;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr EQ binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::EQ;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr NE binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::NE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LT binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LT;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr GT binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::GT;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LE binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr GE binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::GE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LOG_AND binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LOG_AND;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LOG_OR binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LOG_OR;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr AMP binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::BIN_AND;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr PIPE binary_expr
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::BIN_OR;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
;

// унарные выражения
unary_expr:
    primary_expr
    | MINUS unary_expr %prec UMINUS
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::UN_OP;
        e->un_op_ = ast::unary_op::NEG;
        e->lhs_ = $2;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | BANG unary_expr // !x
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::UN_OP;
        e->un_op_ = ast::unary_op::LOG_NOT;
        e->lhs_ = $2;
        e->line_ = @1.begin.line;
        $$ = e;
    }
;



primary_expr:
    INT_LIT
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::INT_LIT;
        e->int_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | BOOL_LIT
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BOOL_LIT;
        e->bool_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | CHAR_LIT
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::CHAR_LIT;
        e->char_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | STRING_LIT
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::STRING_LIT;
        e->string_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | IDENTIFIER
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::VAR_REF;
        e->name_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    // вызов функции: foo(a, b, c)
    | IDENTIFIER LPAREN opt_arg_list RPAREN
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::CALL;
        e->name_ = $1;
        e->args_ = $3;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    // индексация: arr[i]
    | IDENTIFIER LBRACKET expr RBRACKET
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::INDEX;
        e->line_ = @1.begin.line;

        auto base = std::make_shared<ast::expr>();
        base->kind_ = ast::expr_kind::VAR_REF;
        base->name_ = $1;
        base->line_ = @1.begin.line;

        e->lhs_ = base;      // массив
        e->rhs_ = $3;        // индекс
        $$ = e;
    }
    | LPAREN expr RPAREN
    {
        $$ = $2;
    }
;

opt_arg_list:
    %empty              { $$ = {}; }
    | arg_list          { $$ = $1; }
;

arg_list:
    expr
    {
        $$ = { $1 };
    }
    | arg_list COMMA expr
    {
        $1.push_back($3);
        $$ = $1;
    }
;

%%