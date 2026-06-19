#include <Codegen/Codegen.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/Parser.hpp>
#include <Infra/Arena.hpp>

void usage(){
    std::cout << "minicc <file> [-o path]\n";
    std::exit(0);
}

int main(int argc, char *argv[])
{
    std::string_view in_file;
    std::string out_file;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        if(arg == "-h")
            usage();
        else if(arg == "-o")
            out_file = argv[++i];
        else
            in_file = arg;
    }
    if (out_file.empty())
        out_file = std::string{in_file.substr(0, in_file.rfind("."))} + ".s";

    Lexer lexer;
    auto toks = lexer.tokenizeFile(in_file.data());
    Arena arena{1024 * 1024 * 4};
    Sema sema{arena};
    Parser parser{toks, sema, arena};
    auto prog = parser.parseProgram();
    Codegen{sema}.genProg(prog, out_file);
}