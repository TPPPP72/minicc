#include <Lexer.hpp>
#include <AST.hpp>
#include <minicc.hpp>

int main(int argc, char *argv[])
{
    Lexer lexer;
    auto toks = lexer.tokenize(argv[1]);
    AST ast{toks};
    auto node = ast.stmt();
}