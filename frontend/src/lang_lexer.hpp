#pragma once

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include <sstream>
#include <string>

#include "parser.tab.hpp"

namespace yy {
    class lang_lexer : public yyFlexLexer {
    private:
        std::istringstream* istr_ = nullptr;
    public:
        explicit lang_lexer(std::istream& input) : yyFlexLexer(&input) {}

        explicit lang_lexer(const std::string& input) : yyFlexLexer() {
            istr_ = new std::istringstream(input);
            yyFlexLexer::switch_streams(istr_);
        }
        ~lang_lexer() override { delete istr_; }

        yy::Parser::symbol_type yylex(yy::Parser::location_type* yylloc);

    };
}