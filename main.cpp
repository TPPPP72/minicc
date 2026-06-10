#include <Codegen/Codegen.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/Parser.hpp>

int main(int argc, char *argv[])
{
    Lexer lexer;
    auto toks = lexer.tokenize(argv[1]);
    Sema sema;
    Parser parser{toks, sema};
    auto prog = parser.parseProgram();
    Codegen{}.genProg(prog);
}