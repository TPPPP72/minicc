#pragma once

#include <cstdint>
#include <string>
#include <string_view>

enum class TokenKind
{
    INVALID,
    KEYWORD,
    IDENT,
    PUNCT,
    NUM,
    STR,
    OPERATOR,
    ASSIGN,
    ENDF
};

struct Token
{
    std::string string_val;
    std::int64_t val;
    std::string_view source;
    TokenKind kind;
    std::uint32_t loc;
    std::uint32_t len;
    std::uint32_t line_num;
    std::uint32_t col_num;

    Token() = default;
    Token(std::string_view s, TokenKind k, std::uint32_t loc, std::uint32_t len, std::uint32_t line, std::uint32_t col) : source(s), kind(k), loc(loc), len(len), line_num(line), col_num(col) {}
    std::string_view getContent() const { return source.substr(loc, len); }
};