#pragma once

#include <AST/Object.hpp>
#include <cstdint>

enum class NodeType
{
    BLOCK,
    RETURN,
    EXPR_STMT,
    ASSIGN,
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
    Node *next;
    Node *lhs;
    Node *rhs;
    Node *body;
    Object *var;
    std::int32_t val;

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