%option c++
%option yyclass="yy::lang_lexer"
%option noyywrap
%option batch
%option never-interactive
%option yylineno
%option noinput
%option nounput

%{
#include <cstring>
#include <string>
#include "parser.tab.hpp"
#include "lang_lexer.hpp"



#undef YY_DECL
#define YY_DECL yy::Parser::symbol_type yy::lang_lexer::yylex(yy::Parser::location_type* loc)

#undef yyterminate
#define yyterminate() return yy::Parser::make_EOF(*loc)
#define YY_USER_ACTION \
    loc->begin.line = loc->end.line = yylineno; \
    loc->begin.column = 1; \
    loc->end.column = yyleng;


int lexical_errors = 0;

void yyerror(const std::string* msg);
using Token = yy::Parser::token;
%}

DIGIT [0-9]
LETTER [a-zA-Z_]
ID {LETTER}({LETTER}|{DIGIT})*
WHITESPACE [ \t\r\n]+


%%


"i32"       { return yy::Parser::make_I32(*loc); }
"bool"      { return yy::Parser::make_BOOL(*loc); }
"char"      { return yy::Parser::make_CHAR(*loc); }
"void"      { return yy::Parser::make_VOID(*loc); }
"ptr$"      { return yy::Parser::make_PTR(*loc); }

"if"        { return yy::Parser::make_IF(*loc); }
"elif"      { return yy::Parser::make_ELIF(*loc); }
"else"      { return yy::Parser::make_ELSE(*loc); }
"while"     { return yy::Parser::make_WHILE(*loc); }
"return"    { return yy::Parser::make_RETURN(*loc); }
"break"     { return yy::Parser::make_BREAK(*loc); }
"continue"  { return yy::Parser::make_CONTINUE(*loc); }


"true"      { return yy::Parser::make_BOOL_LIT(true, *loc); }
"false"     { return yy::Parser::make_BOOL_LIT(false, *loc); }

"-"?{DIGIT}+ {
    long long v = std::atoll(yytext);
    return yy::Parser::make_INT_LIT(v, *loc);
}


\'([^'\\]|\\.)\' {      // Символьный литерал: 'a', '\n', '\''
    char c = yytext[1];
    if (c == '\\') {
        switch (yytext[2]) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            default: c = yytext[2];
        }
    }
    return yy::Parser::make_CHAR_LIT(c, *loc);
}


\"([^"\\]|\\.)*\" {     // Строковый литерал: "hello", "a\nb" и тд
    std::string s(yytext + 1, yyleng - 2);
    return yy::Parser::make_STRING_LIT(s, *loc);
}

{ID} {      // идентификатор
    return yy::Parser::make_IDENTIFIER(std::string(yytext), *loc);
}


"=="    { return yy::Parser::make_EQ(*loc); }
"!="    { return yy::Parser::make_NE(*loc); }
"<="    { return yy::Parser::make_LE(*loc); }
">="    { return yy::Parser::make_GE(*loc); }
"&&"    { return yy::Parser::make_LOG_AND(*loc); }
"||"    { return yy::Parser::make_LOG_OR(*loc); }
"+="    { return yy::Parser::make_PLUS_EQ(*loc); }
"-="    { return yy::Parser::make_MINUS_EQ(*loc); }
"*="    { return yy::Parser::make_STAR_EQ(*loc); }
"/="    { return yy::Parser::make_SLASH_EQ(*loc); }

"+"     { return yy::Parser::make_PLUS(*loc); }
"-"     { return yy::Parser::make_MINUS(*loc); }
"*"     { return yy::Parser::make_STAR(*loc); }
"/"     { return yy::Parser::make_SLASH(*loc); }
"&"     { return yy::Parser::make_AMP(*loc); }
"|"     { return yy::Parser::make_PIPE(*loc); }
"!"     { return yy::Parser::make_BANG(*loc); }
"<"     { return yy::Parser::make_LT(*loc); }
">"     { return yy::Parser::make_GT(*loc); }
"="     { return yy::Parser::make_ASSIGN(*loc); }
"("     { return yy::Parser::make_LPAREN(*loc); }
")"     { return yy::Parser::make_RPAREN(*loc); }
"{"     { return yy::Parser::make_LBRACE(*loc); }
"}"     { return yy::Parser::make_RBRACE(*loc); }
"["     { return yy::Parser::make_LBRACKET(*loc); }
"]"     { return yy::Parser::make_RBRACKET(*loc); }
";"     { return yy::Parser::make_SEMICOLON(*loc); }
","     { return yy::Parser::make_COMMA(*loc); }


{WHITESPACE}    {  } // пробелы
"//"[^\n]*      {  } // однострочные комментарии



. {
    std::cerr << "LEXICAL ERROR (line " << yylineno << "): invalid character '" << yytext << "'" << std::endl;
    lexical_errors++;
}

%%
