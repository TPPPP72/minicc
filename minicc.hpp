#pragma once

#include <cstdint>
#include <string_view>

enum class TokenType
{
    INVALID,
    KEYWORD,
    PUNCT,
    VAR,
    NUM,
    OPERATOR,
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