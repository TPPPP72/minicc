#pragma once

#include <AST/Type.hpp>
#include <Lexer/Token.hpp>
#include <Scope/Variable.hpp>
#include <cstdint>
#include <vector>

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
    DEREF,
    FUNCALL
};

struct alignas(64) Node
{
    Token tok;
    NodeKind kind;
    TypeId type_id{};

    Node(NodeKind k, const Token &t) : tok(t), kind(k), type_id(0) {}
    virtual ~Node() = default;
};

struct alignas(64) NumNode : Node
{
    NumNode(std::int32_t v, const Token &t) : Node(NodeKind::NUM, t), val(v) {}

    NumNode(std::int32_t v) : Node(NodeKind::NUM, {}), val(v) {}

    std::int32_t val;
};

struct alignas(64) VarNode : Node
{
    VarNode(Variable *v, const Token &t) : Node(NodeKind::VAR, t), var(v) {}

    VarNode(Variable *v) : Node(NodeKind::VAR, {}), var(v) {}

    Variable *var;
};

struct alignas(128) UnaryNode : Node
{
    UnaryNode(NodeKind k, Node *l, const Token &t) : Node(k, t), lhs(l) {}

    UnaryNode(NodeKind k, Node *l) : Node(k, {}), lhs(l) {}

    ~UnaryNode() override
    {
        delete lhs;
    }

    Node *lhs;
};

struct alignas(128) BinaryNode : Node
{
    BinaryNode(NodeKind k, Node *l, Node *r, const Token &t)
        : Node(k, t), lhs(l), rhs(r) {}

    BinaryNode(NodeKind k, Node *l, Node *r)
        : Node(k, {}), lhs(l), rhs(r) {}

    ~BinaryNode() override
    {
        delete lhs;
        delete rhs;
    }

    Node *lhs;
    Node *rhs;
};

struct alignas(128) ExprStmtNode : Node
{
    ExprStmtNode(Node *e, const Token &t)
        : Node(NodeKind::EXPR_STMT, t), expr(e) {}

    ~ExprStmtNode() override
    {
        delete expr;
    }

    Node *expr;
};

struct alignas(128) BlockNode : Node
{
    BlockNode(const Token &t) : Node(NodeKind::BLOCK, t) {}

    ~BlockNode() override
    {
        for (auto s : stmts)
            delete s;
    }

    std::vector<Node *> stmts;
};

struct alignas(128) ReturnNode : Node
{
    ReturnNode(Node *e, const Token &t)
        : Node(NodeKind::RETURN, t), expr(e) {}

    ~ReturnNode() override
    {
        delete expr;
    }

    Node *expr;
};

struct alignas(128) IfNode : Node
{
    IfNode(const Token &t)
        : Node(NodeKind::IF, t), cond(nullptr), then(nullptr), els(nullptr) {}

    ~IfNode() override
    {
        delete cond;
        delete then;
        if (els)
            delete els;
    }

    Node *cond;
    Node *then;
    Node *els;
};

struct alignas(128) ForNode : Node
{
    ForNode(const Token &t)
        : Node(NodeKind::FOR, t), init(nullptr), cond(nullptr), inc(nullptr), then(nullptr) {}

    ~ForNode() override
    {
        if (init)
            delete init;
        if (cond)
            delete cond;
        if (inc)
            delete inc;
        delete then;
    }

    Node *init;
    Node *cond;
    Node *inc;
    Node *then;
};

struct FuncCallNode : Node
{
    FuncCallNode(std::string_view name, const Token &t)
        : Node(NodeKind::FUNCALL, t), func_name(name) {}

    FuncCallNode(std::string_view name)
        : Node(NodeKind::FUNCALL, {}), func_name(name) {}

    ~FuncCallNode() override
    {
        for (auto arg : args)
            delete arg;
    }

    std::string_view func_name;
    std::vector<Node *> args;
};