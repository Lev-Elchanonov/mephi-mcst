#include <fstream>
#include <iostream>

#include "lang_lexer.hpp"
#include "parser.tab.hpp"
#include "ast.hpp"

// Внешние переменные, определённые в lexer.l и parser.y
extern int lexical_errors;
extern int syntax_errors;
extern std::shared_ptr<ast::program> program;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }

    // Создаём программу заранее, чтобы парсер её заполнял
    program = std::make_shared<ast::program>();

    yy::lang_lexer lexer(file);
    yy::Parser parser(lexer);
    THE_LEXER = &lexer;
    int result = parser.parse();

    if (lexical_errors > 0 || syntax_errors > 0) {
        std::cerr << "Errors: lexical=" << lexical_errors
                  << ", syntax=" << syntax_errors << "\n";
        return 1;
    }

    std::cout << "Parsing successful. Functions: "
              << program->functions_.size() << "\n";
    return 0;
}