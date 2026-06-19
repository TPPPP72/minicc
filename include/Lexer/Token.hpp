#pragma once

#include <string_view>
#include <cstdint>
#include <string>

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

struct alignas(64) Token
{
    std::string string_val;
    std::string_view source;
    TokenKind kind;
    std::uint32_t loc;
    std::uint32_t len;

    Token() = default;
    Token(std::string_view s, TokenKind k, std::uint32_t loc, std::uint32_t len) : source(s), kind(k), loc(loc), len(len) {}
    std::string_view getContent() const { return source.substr(loc, len); }
};