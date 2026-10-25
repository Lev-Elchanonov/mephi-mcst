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
#include "codegen_c.hpp"

extern std::shared_ptr<ast::program> program;
extern int lexical_errors;
extern int syntax_errors;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.lang> <output.c>\n";
        return 1;
    }

    std::string input_name = argv[1];
    std::string out_name = argv[2];

    std::ifstream file(input_name);
    if (!file) {
        std::cerr << "Cannot open input file: " << input_name << "\n";
        return 1;
    }

    program = std::make_shared<ast::program>();
    yy::lang_lexer lexer(file);
    yy::Parser parser(lexer);
    parser.parse();

    if (lexical_errors > 0 || syntax_errors > 0) {
        std::cerr << "Errors: lexical=" << lexical_errors
                  << ", syntax=" << syntax_errors << "\n";
        return 1;
    }

    sema::analyzer a;
    if (!a.analyze(program)) {
        std::cerr << "Semantic analysis failed\n";
        return 1;
    }

    ir::translation_unit unit;
    using ir_builder_t = ir::builder<ast::program, ast::func_decl, ast::stmt, ast::expr>;
    ir_builder_t b;
    b.build(program, unit);

    ir::verifier v;
    if (!v.verify(unit)) {
        std::cerr << "IR verification failed\n";
        return 1;
    }

    lir::lir_unit lunit;
    lir::lowerer lower;
    lower.lower(unit, lunit);

    std::ofstream out(out_name);
    if (!out) {
        std::cerr << "Cannot create output file: " << out_name << "\n";
        return 1;
    }

    std::cout << "Generating C code...\n";
    cgen::c_generator gen;
    gen.generate(lunit, out);

    std::cout << "Generated C code successfully: " << out_name << "\n";
    return 0;
}