%require "3.2"
%language "c++"
%defines "parser.tab.hpp"
%output "parser.tab.cpp"

// тут код, который будет в hpp файле parser.tab.hpp
// он тут нужен для подключения в ast.hpp

// code requires попадет в parser.tab.hpp. Это надо чтобы бизон мог сгенерировать объявления,
// в которых упоминается lang_lexer
%code requires{
    #include <memory>
    #include <string>
    #include <vector>
    #include "ast.hpp"

    namespace yy { class lang_lexer; }
}



// тут код, который попадает в parser.tab.cpp
%code {
    #include <iostream>
    #include "lang_lexer.hpp"
    #include "ast.hpp"

    int syntax_errors = 0;

    std::shared_ptr<ast::program> program;

    // функция вывода синтаксической ошибки
    void yy::Parser::error(const location_type& loc, const std::string& msg){
        std::cerr << "SYNTAX ERROR (line " << loc.begin.line<< "): " << msg << std::endl;
        syntax_errors++;
    }

    // yylex - функция, котрую вызывает бизон, когда ему нужен следующий токен
    static int yylex(yy::Parser::semantic_type* yylval, yy::location* loc, yy::lang_lexer& lexer) {
            return lexer.yylex(yylval, loc);
    }

}

// Тут настройки парсера
%define api.parser.class { Parser } // задает имя класса парсера
%define api.namespace { yy } // задает его пространство имен
%define api.value.type variant // задает тип хранения значений токенов и нетерминалов (есть INT_LIT, STRING_LIT и тд)
%define parse.error verbose     // подробные сообщения об ошибках
%define parse.trace  // трассировка парсера



%locations // для отслеживания позиции

// передача лексера в парсер
%lex-param  { yy::lang_lexer& lexer } // добавляет lexer в вызов yylex
%parse-param { yy::lang_lexer& lexer } // добавялет параметр lexer в конструктор парсера


// Объявления всех токенов
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
// param_list - непустой список параметров (i32 a, bool b) (сигнатура функции)
// opt_param_list - список или пусто (для foo() без параметров)

%type <std::shared_ptr<ast::func_decl>> func_def
// func_def - одно объявление функции целиком

%type <std::vector<size_t>> array_dims array_dims_nonempty
// array_dims - размерности массива (может быть 0, если переменная типа i32 a;)
// array_dims_nonempty - размерности либо одномерного массива, либо многомерного


// задание приоритетов операторов
// тут задание идет от низкого к высокому
// логические - самые слабые, потом сравнения, потом арифметика, потом унарные, потом скобки и вызовы
// %left - это для a OP b OP c -> (a OP b) OP c
// %right - это для !!x -> !(!x), --x -> -(-x)
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

// старт синтаксического анализа
program:
    func_def_list // натыкаемся на список функций. Потом будем его раскрывать в другом правиле
    {

    }
;

// список объявлений функций
func_def_list:
    %empty // список пуст
    | func_def_list func_def // добавить функцию в список функций
    {
        program->functions_.push_back($2);
    }
    | func_def_list error func_def // восстановление если в функции ошибка
    {
        // проглотили мусор и нашли следующую функцию
        program->functions_.push_back($3);
        yyerrok;
        yyclearin;
    }

;

// объявление одной функции
func_def:
    type_spec IDENTIFIER LPAREN opt_param_list RPAREN block // i32 foo(i32 a, bool b) block
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
    | param_list COMMA param // добавляем все параметры по очереди в param_list
    {
        $1.push_back($3);
        $$ = $1;
    }
    | param_list error COMMA // восстановления в случае синтаксической ошибки
    {
        yyerrok;
        yyclearin;
        $$ = $1;
    }
    | param_list error RPAREN
    {
        yyerrok;
        $$ = $1;
    }
;

// правило для одного конкретного параметра
param:
    type_spec IDENTIFIER
    {
        $$ = ast::param{$1, $2};
    }
;

// нетерминал, описывающий тип объявления
type_spec:
    base_type
    {
        $$ = $1;
    }
    | PTR type_spec // если у нас ptr$ i32
    {
        $$ = ast::type{ast::type_kind::POINTER, std::make_shared<ast::type>($2), 0};
    }
;

// правило для базового типа (i32, bool, char, void)
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

// список правил
stmt_list:
    %empty // либо правил нет if (...) { пусто }
    {
        $$ = {};
    }
    | stmt_list stmt // сворачиваем правила в список правил
    {
        $1.push_back($2);
        $$ = $1;
    }
    | stmt_list error SEMICOLON // если ошибка, то восстанавливаем автомат
    {
        // кушаем плохую инструкцию до ;
        yyerrok;
        yyclearin;
        $$ = $1;
    }
;

// правило может быть простой конструкцией и составной
// simple_stmt -> i32 x = 5, x = 10, foo(), continue
// compound_stmt -> if (x > 0) { ... }, { ... }, while
stmt:
    simple_stmt             { $$ = $1; }
    | compound_stmt         { $$ = $1; }
;

// простые конструкции
simple_stmt:
    // объявления переменной
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
    | primary_expr ASSIGN expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN;
        st->line_ = @2.begin.line;
        st->target_ = $1;
        st->value_ = $3;
        $$ = st;
    }

    // +=
    | primary_expr PLUS_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::ADD;
        st->line_ = @2.begin.line;
        st->target_ = $1;
        st->value_ = $3;
        $$ = st;
    }

    // *=
    | primary_expr STAR_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::MUL;
        st->line_ = @2.begin.line;
        st->target_ = $1;
        st->value_ = $3;
        $$ = st;
    }

    // /=
    | primary_expr SLASH_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::DIV;
        st->line_ = @2.begin.line;
        st->target_ = $1;
        st->value_ = $3;
        $$ = st;
    }

    // -=
    | primary_expr MINUS_EQ expr SEMICOLON
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::ASSIGN_OP;
        st->assign_op_ = ast::binary_op::SUB;
        st->line_ = @2.begin.line;
        st->target_ = $1;
        st->value_ = $3;
        $$ = st;
    }
;

// объявление переменной
var_decl:
    type_spec IDENTIFIER array_dims // i32 x, i32 arr[3] и тд
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::VAR_DECL;
        st->name_ = $2;
        st->line_ = @2.begin.line;
        st->column_ = @2.begin.column;

        if ($3.empty()) { // есди нет размерностей то это просто переменная i32 x, к примеру
            st->var_type_ = $1;
        } else { // есди есть размерности, то надо обработать как массив
            ast::type arr = $1;
            for (auto it = $3.rbegin(); it != $3.rend(); ++it) {
                // добавляем по размерности начиная с конца (массив массивов массивов и тд)
                arr = ast::type{ast::type_kind::ARRAY, std::make_shared<ast::type>(arr), *it};
            }
            st->var_type_ = arr;
        }
        $$ = st;
    }
    | type_spec IDENTIFIER array_dims ASSIGN expr //i32 dims[5] = x; (присваивание)
    {
        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::VAR_DECL;
        st->name_ = $2;
        st->line_ = @2.begin.line;
        st->column_ = @2.begin.column;

        if ($3.empty()) {
            st->var_type_ = $1;
        } else {
            ast::type arr = $1;
            for (auto it = $3.rbegin(); it != $3.rend(); ++it) {
                arr = ast::type{ast::type_kind::ARRAY,
                                std::make_shared<ast::type>(arr),
                                *it};
            }
            st->var_type_ = arr;
        }

        st->init_value_ = $5; // добавляется начальное значение переменной
        $$ = st;
    }
;


// размерности составных массивов (обычных тоже)
array_dims:
    %empty      { $$ = {}; }
    | array_dims_nonempty
    {
        $$ = $1;
    }
;

// непустые размерности массивов
array_dims_nonempty:
    LBRACKET INT_LIT RBRACKET // int размерность
    {
        $$ = { (size_t)$2 };
    }
    | array_dims_nonempty LBRACKET INT_LIT RBRACKET // собираем все размерности в список
    {
        $1.push_back((size_t)$3);
        $$ = $1;
    }
    | LBRACKET RBRACKET // если просто [], то размер = 0.
    {
        $$ = { 0 };
    }
    | array_dims_nonempty LBRACKET RBRACKET // если уже были собраны размерности и встречается [], то новая размерность имеет size = 0
    {
        $1.push_back(0);
        $$ = $1;
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
    | IF LPAREN expr RPAREN block ELIF LPAREN expr RPAREN block opt_elif_else // if(){} elif(){}
    {
        // разворачиваем elif в else { if (...) { ... } }
        auto inner = std::make_shared<ast::stmt>();
        inner->kind_ = ast::stmt_kind::IF;
        inner->condition_ = $8;
        inner->then_body_ = $10;
        inner->else_body_ = $11;
        inner->line_ = @7.begin.line;

        auto st = std::make_shared<ast::stmt>();
        st->kind_ = ast::stmt_kind::IF;
        st->condition_ = $3;
        st->then_body_ = $5;
        st->else_body_ = { inner };
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
    | ELIF LPAREN expr RPAREN block opt_elif_else // раскрываем еще один elif
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
unary_expr // это для свертки. Любая binary expr сначала unary expr
// например: a + b:
// a -> unary_expr -> binary_expr
// + -> PLUS
// b -> unary_expr -> binary_expr
// a + b -> binary_expr + binary_expr
    | binary_expr PLUS binary_expr // a + b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::ADD;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr MINUS binary_expr // a - b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::SUB;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr STAR binary_expr // a * b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::MUL;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr SLASH binary_expr // a / b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::DIV;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr EQ binary_expr // a == b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::EQ;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr NE binary_expr // a != b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::NE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LT binary_expr // a < b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LT;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr GT binary_expr // a > b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::GT;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LE binary_expr // a <= b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr GE binary_expr // a >= b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::GE;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LOG_AND binary_expr // a && b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LOG_AND;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr LOG_OR binary_expr // a || b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::LOG_OR;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr AMP binary_expr // a & b
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BIN_OP;
        e->bin_op_ = ast::binary_op::BIN_AND;
        e->lhs_ = $1;
        e->rhs_ = $3;
        e->line_ = @2.begin.line;
        $$ = e;
    }
    | binary_expr PIPE binary_expr // a | b
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
// unary_expr это либо primary_expr, либо оператор над unary_expr
    primary_expr
    | MINUS unary_expr %prec UMINUS // - -x
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


// базовое выражение
primary_expr:
    INT_LIT // int литерал
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::INT_LIT;
        e->int_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | BOOL_LIT // bool
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::BOOL_LIT;
        e->bool_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | CHAR_LIT // char
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::CHAR_LIT;
        e->char_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | STRING_LIT // string
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::STRING_LIT;
        e->string_value_ = $1;
        e->line_ = @1.begin.line;
        $$ = e;
    }
    | IDENTIFIER // id (ссылка на переменную)
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
    // индексация: arr[i] или arr[i][j]
    | primary_expr LBRACKET expr RBRACKET
    {
        auto e = std::make_shared<ast::expr>();
        e->kind_ = ast::expr_kind::INDEX;
        e->line_ = @1.begin.line;

        e->lhs_ = $1;
        e->rhs_ = $3;
        $$ = e;
    }
    | LPAREN expr RPAREN // (a + b)
    {
        $$ = $2;
    }
;

// необязательный список аргументов вызова функции
opt_arg_list:
    %empty              { $$ = {}; }
    | arg_list          { $$ = $1; }
;

// аргументы вызова функции
arg_list:
    expr // один аргумент
    {
        $$ = { $1 };
    }
    | arg_list COMMA expr // свертка аргументов в список
    {
        $1.push_back($3);
        $$ = $1;
    }
    | arg_list error COMMA // восстановление после ошибки
    {
        yyerrok;
        yyclearin;
        $$ = $1;
    }
;

%%