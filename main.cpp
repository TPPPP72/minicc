#include <Codegen.hpp>
#include <Lexer.hpp>
#include <Parser.hpp>
#include <minicc.hpp>

int main(int argc, char *argv[])
{
    Lexer lexer;
    auto toks = lexer.tokenize(argv[1]);
    Parser parser{toks};
    auto node = parser.parse();
    Codegen{}.generate(node);
}