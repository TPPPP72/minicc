#pragma once

#include <string_view>
#include <cstdint>

enum class TokenType
{
    INVALID,
    KEYWORD,
    IDENT,
    PUNCT,
    NUM,
    OPERATOR,
    ASSIGN,
    ENDF
};

struct alignas(32) Token
{
    std::string_view source;
    TokenType type;
    std::uint32_t loc;
    std::uint32_t len;

    std::string_view getContent() const { return source.substr(loc, len); }
};