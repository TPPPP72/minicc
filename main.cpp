#include <Codegen/Codegen.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/Parser.hpp>
#include <Infra/Arena.hpp>

int main(int argc, char *argv[])
{
    Lexer lexer;
    auto toks = lexer.tokenizeFile(argv[1]);
    Arena arena{1024 * 1024 * 4};
    Sema sema{arena};
    Parser parser{toks, sema, arena};
    auto prog = parser.parseProgram();
    Codegen{sema}.genProg(prog);
}