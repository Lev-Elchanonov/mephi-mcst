#include <fstream>
#include <iostream>
#include <string>

#include "lang_lexer.hpp"
#include "parser.tab.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "ir_structs.hpp"
#include "ir_builder.hpp"
#include "ir_verifier.hpp"
#include "lir_structs.hpp"
#include "lir_lowering.hpp"
#include "lir_dump.hpp"

extern std::shared_ptr<ast::program> program;
extern int lexical_errors;
extern int syntax_errors;

int main(int argc, char** argv) {
    std::string input;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        input = a;
    }
    if (input.empty()) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }
    std::ifstream file(input);
    if (!file) { std::cerr << "Cannot open " << input << "\n"; return 1; }

    program = std::make_shared<ast::program>();
    yy::lang_lexer lexer(file);
    yy::Parser parser(lexer);
    parser.parse();
    if (lexical_errors || syntax_errors) return 1;

    sema::analyzer a;
    if (!a.analyze(program)) return 1;

    ir::translation_unit unit;
    using ir_builder_t = ir::builder<ast::program, ast::func_decl, ast::stmt, ast::expr>;
    ir_builder_t b;
    b.build(program, unit);

    ir::verifier v;
    if (!v.verify(unit)) return 1;

    lir::lir_unit lunit;
    lir::lowerer lower;
    lower.lower(unit, lunit);

    lir::dump_lir(std::cout, lunit);
    return 0;
}