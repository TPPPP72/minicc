#pragma once

#include <AST/Object.hpp>
#include <Lexer/Token.hpp>
#include <cstdint>

enum class NodeType
{
    BLOCK,
    RETURN,
    EXPR_STMT,
    ASSIGN,
    IF,
    FOR,
    NEG,
    ADD,
    VAR,
    SUB,
    MUL,
    DIV,
    MOD,
    NUM,
    EQ,
    NE,
    LE,
    LT,
    GE,
    GT
};

struct Node
{
    NodeType type;
    Token tok;
    Node *next{nullptr};
    Node *lhs{nullptr};
    Node *rhs{nullptr};
    Node *body{nullptr};
    Node *cond{nullptr};
    Node *then{nullptr};
    Node *els{nullptr};
    Node *init{nullptr};
    Node *inc{nullptr};
    Object *var{nullptr};
    std::int32_t val{};

    Node() = default;

    /// Type Only
    Node(NodeType ty) : type(ty) {}

    /// Binary
    Node(NodeType ty, Node *l, Node *r) : type(ty), lhs(l), rhs(r) {}

    /// Unary
    Node(NodeType ty, Node *expr) : type(ty), lhs(expr) {}

    /// Num
    Node(std::int32_t v) : type(NodeType::NUM), val(v) {}

    /// Var
    Node(Object *v) : type(NodeType::VAR), var(v) {}
};