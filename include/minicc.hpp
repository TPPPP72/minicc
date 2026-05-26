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

enum class NodeType
{
    EXPR_STMT,
    NEG,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NUM,
};

struct Node
{
    NodeType type;
    Node *next;
    Node *lhs;
    Node *rhs;
    std::int32_t val;

    Node() = default;

    /// Binary
    Node(NodeType ty, Node *l, Node *r) : type(ty), lhs(l), rhs(r) {}

    /// Unary
    Node(NodeType ty, Node *expr) : type(ty), lhs(expr) {}

    /// Num
    Node(std::int32_t v) : type(NodeType::NUM), val(v) {}
};