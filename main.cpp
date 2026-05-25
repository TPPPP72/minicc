#include "Lexer.hpp"
#include "minicc.hpp"

std::string_view trans(TokenType ty)
{
    if (ty == TokenType::INVALID)
        return "invalid";
    if (ty == TokenType::KEYWORD)
        return "keyword";
    if (ty == TokenType::PUNCT)
        return "punct";
    if (ty == TokenType::VAR)
        return "var";
    if (ty == TokenType::NUM)
        return "num";
    if (ty == TokenType::OPERATOR)
        return "operator";
    if (ty == TokenType::ENDF)
        return "endf";
    return "null";
}

int main(int argc, char *argv[])
{
    Lexer lexer;
    auto tok = lexer.tokenize(argv[1]);

    while (tok.getToken().type != TokenType::ENDF)
    {
        std::cout << tok.getToken().getContent() << ' ' << trans(tok.getToken().type) << '\n';
        tok.skipToken();
    }
}