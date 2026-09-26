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
#define YY_DECL int yy::lang_lexer::yylex(yy::Parser::semantic_type* yylval, yy::location* loc)


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


"i32"       { return Token::I32; }
"bool"      { return Token::BOOL; }
"char"      { return Token::CHAR; }
"void"      { return Token::VOID; }
"ptr$"      { return Token::PTR; }

"if"        { return Token::IF; }
"elif"      { return Token::ELIF; }
"else"      { return Token::ELSE; }
"while"     { return Token::WHILE; }
"return"    { return Token::RETURN; }
"break"     { return Token::BREAK; }
"continue"  { return Token::CONTINUE; }


"true"      { yylval->build<bool>(true);  return Token::BOOL_LIT; }
"false"     { yylval->build<bool>(false); return Token::BOOL_LIT; }

"-"?{DIGIT}+ {
    long long v = std::atoll(yytext);
    yylval->build<long long>(v);
    return Token::INT_LIT;
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
    yylval->build<char>(c);
    return Token::CHAR_LIT;
}


\"([^"\\]|\\.)*\" {     // Строковый литерал: "hello", "a\nb" и тд
    std::string s(yytext + 1, yyleng - 2); // чтобы не учитывать ковычки в "hello", сдвигаем на 1 и leng - 2
    yylval->build<std::string>(s);
    return Token::STRING_LIT;
}

{ID} {      // идентификатор
    yylval->build<std::string>(std::string(yytext));
    return Token::IDENTIFIER;
}


"=="    { return Token::EQ; }
"!="    { return Token::NE; }
"<="    { return Token::LE; }
">="    { return Token::GE; }
"&&"    { return Token::LOG_AND; }
"||"    { return Token::LOG_OR; }
"+="    { return Token::PLUS_EQ; }
"-="    { return Token::MINUS_EQ; }
"*="    { return Token::STAR_EQ; }
"/="    { return Token::SLASH_EQ; }

"+"     { return Token::PLUS; }
"-"     { return Token::MINUS; }
"*"     { return Token::STAR; }
"/"     { return Token::SLASH; }
"&"     { return Token::AMP; }
"|"     { return Token::PIPE; }
"!"     { return Token::BANG; }
"<"     { return Token::LT; }
">"     { return Token::GT; }
"="     { return Token::ASSIGN; }
"("     { return Token::LPAREN; }
")"     { return Token::RPAREN; }
"{"     { return Token::LBRACE; }
"}"     { return Token::RBRACE; }
"["     { return Token::LBRACKET; }
"]"     { return Token::RBRACKET; }
";"     { return Token::SEMICOLON; }
","     { return Token::COMMA; }


{WHITESPACE}    {  } // пробелы
"//"[^\n]*      {  } // однострочные комментарии



. {
    std::cerr << "LEXICAL ERROR (line " << yylineno << "): invalid character '" << yytext << "'" << std::endl;
    lexical_errors++;
}

%%
