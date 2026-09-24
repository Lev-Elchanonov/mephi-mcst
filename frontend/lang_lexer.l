%option c++
%option yyclass="yy::ПОКА НЕ ПРИДУМАЛ"
%option noyywrap
%option batch
%option never-interactive
%option yylineno
%option noinput
%option nounput

%{
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sstream>


// тут какой то код

// тут пара инклудов

int error_count = 0;

void yyerror(const std::string* msg);
using Token = yy::LangParser::token;


DIGIT [0-9]
LETTER [a-zA-Z]
WHITESPACE [ \t\r\n]+

%%


"i32"           {return Token::TOKEN_TYPE_INT;}
"bool"          {return Token::TOKEN_TYPE_BOOL;}
"char"          {return Token::TOKEN_TYPE_CHAR;}

"main"          {yylval->build<std::string>("main"); return Token::TOKEN_MAIN;}


"while"         {return Token::TOKEN_TYPE_WHILE;}
"if"            {return Token::TOKEN_TYPE_IF;}
"return"        {return Token::TOKEN_TYPE_RETURN;}

"+"             {return Token::TOKEN_PLUS;}
"-"             {return Token::TOKEN_MINUS;}
"*"             {return Token::TOKEN_MULT;}
"/"             {return Token::TOKEN_DIV;}
"||"            {return Token::TOKEN_OR;}
"&&"            {return Token::TOKEN_AND;}




"-"?{DIGIT}+            {
                            int val = std::stoi(yytext);
                            yylval->build<int>(val);
                            return Token::TOKEN_TYPE_INT;
                        }

{LETTER}[a-zA-Z0-9_]*   {
                            yylval->build<std::string>(yytext);
                            return Token::TOKEN_IDENTIFIER;
                        }


{WHITESPACE}            {   }




.                       {
                            std::cerr << "LEXICAL ERROR (line " << yylineno << "): invalid character '" << yytext << "'" << std::endl;
                            error_count++;
                        }

%%
