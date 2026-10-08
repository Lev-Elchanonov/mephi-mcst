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
#include "ir_cfg_builder.hpp"
#include "ir_dump.hpp"

extern std::shared_ptr<ast::program> program;
extern int lexical_errors;
extern int syntax_errors;

int main(int argc, char** argv) {
    bool emit_ir = false;
    bool emit_cfg = false;
    bool cfg_png = false;
    std::string input;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--ir") emit_ir = true;
        else if (a == "--cfg") emit_cfg = true;
        else if (a == "--cfg-png") cfg_png = true;
        else input = a;
    }

    if (input.empty()) {
        std::cerr << "Usage: " << argv[0] << " -DIR <file.lang>\n";
        return 1;
    }

    std::ifstream file(input);
    if (!file) {
        std::cerr << "Cannot open " << input << "\n";
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
    ir::builder<ast::program, ast::func_decl, ast::stmt, ast::expr> b;
    b.build(program, unit);


    ir::verifier v;
    if (!v.verify(unit)) {
        std::cerr << "IR verification failed\n";
        return 1;
    }


    if (emit_ir) ir::dump(std::cout, unit);

    ir::cfg_builder cfg;
    for (auto& f : unit.functions_) cfg.build(*f);

    if (emit_cfg) {
        for (auto& f : unit.functions_) ir::dump_cfg(std::cout, *f);
    }

    if (cfg_png) {
        for (auto& f : unit.functions_) {
            open_cfg_png(*f);
        }
    }

    if (!emit_ir && !emit_cfg) {
        std::cout << "IR OK, functions: " << unit.functions_.size() << "\n";
    }
    return 0;
}