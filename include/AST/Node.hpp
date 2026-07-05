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
    COMMA,
    NUM,
    EQ,
    NE,
    LE,
    LT,
    GE,
    GT,
    ADDR,
    DEREF,
    FUNCALL,
    TYPECAST,
    MEMBER
};

struct Node
{
    Token tok;
    NodeKind kind;
    TypeId type_id{};

    Node(NodeKind k, const Token &t) : tok(t), kind(k), type_id(0) {}
    Node(NodeKind k, TypeId tid, const Token &t) : tok(t), kind(k), type_id(tid) {}
    virtual ~Node() = default;
};

struct NumNode : Node
{
    NumNode(std::int64_t v, const Token &t) : Node(NodeKind::NUM, t), val(v) {}

    NumNode(std::int64_t v) : Node(NodeKind::NUM, {}), val(v) {}

    std::int64_t val;
};

struct VarNode : Node
{
    VarNode(Variable *v, const Token &t) : Node(NodeKind::VAR, t), var(v) {}

    VarNode(Variable *v) : Node(NodeKind::VAR, {}), var(v) {}

    Variable *var;
};

struct MemberNode : Node
{
    MemberNode(Node *l, int of, const Token &t) : Node(NodeKind::MEMBER, t), lhs(l), offset(of) {}

    MemberNode(Node *l, int of) : Node(NodeKind::MEMBER, {}), lhs(l), offset(of) {}

    Node *lhs;
    int offset;
};

struct UnaryNode : Node
{
    UnaryNode(NodeKind k, Node *l, const Token &t) : Node(k, t), lhs(l) {}

    UnaryNode(NodeKind k, Node *l) : Node(k, {}), lhs(l) {}

    Node *lhs;
};

struct BinaryNode : Node
{
    BinaryNode(NodeKind k, Node *l, Node *r, const Token &t)
        : Node(k, t), lhs(l), rhs(r) {}

    BinaryNode(NodeKind k, Node *l, Node *r)
        : Node(k, {}), lhs(l), rhs(r) {}

    Node *lhs;
    Node *rhs;
};

struct ExprStmtNode : Node
{
    ExprStmtNode(Node *e, const Token &t)
        : Node(NodeKind::EXPR_STMT, t), expr(e) {}

    Node *expr;
};

struct BlockNode : Node
{
    BlockNode(const Token &t) : Node(NodeKind::BLOCK, t) {}

    std::vector<Node *> stmts;
};

struct ReturnNode : Node
{
    ReturnNode(Node *e, const Token &t)
        : Node(NodeKind::RETURN, t), expr(e) {}

    Node *expr;
};

struct IfNode : Node
{
    IfNode(const Token &t)
        : Node(NodeKind::IF, t), cond(nullptr), then(nullptr), els(nullptr) {}

    Node *cond;
    Node *then;
    Node *els;
};

struct ForNode : Node
{
    ForNode(const Token &t)
        : Node(NodeKind::FOR, t), init(nullptr), cond(nullptr), inc(nullptr), then(nullptr) {}

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

    std::string_view func_name;
    std::vector<Node *> args;
};

struct TypeCastNode : Node
{
    TypeCastNode(Node *l, TypeId to, const Token &t) : Node(NodeKind::TYPECAST, to, t), expr(l) {}

    TypeCastNode(Node *l, TypeId to) : Node(NodeKind::TYPECAST, to, {}), expr(l) {}

    Node *expr;
};