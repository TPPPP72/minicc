#pragma once

#include <AST/Object.hpp>
#include <AST/Type.hpp>
#include <Lexer/Token.hpp>
#include <cstdint>

enum class NodeKind
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
    GT,
    ADDR,
    DEREF
};

struct alignas(128) Node
{
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
    NodeKind kind;
    TypeId type_id{};
    std::int32_t val{};

    Node() = default;

    /// Type Only
    Node(NodeKind ty) : kind(ty) {}

    /// Binary
    Node(NodeKind ty, Node *l, Node *r) : kind(ty), lhs(l), rhs(r) {}

    /// Unary
    Node(NodeKind ty, Node *expr) : kind(ty), lhs(expr) {}

    /// Num
    Node(std::int32_t v) : kind(NodeKind::NUM), val(v) {}

    /// Var
    Node(Object *v) : kind(NodeKind::VAR), var(v) {}
};