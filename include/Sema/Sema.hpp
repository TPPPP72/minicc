#pragma once

#include <AST/Node.hpp>
#include <AST/Type.hpp>
#include <Diag/Diag.hpp>
#include <Infra/Arena.hpp>
#include <Sema/TypeContext.hpp>

class Sema
{
public:
    Sema(Arena &arena) : m_arena(arena) {}

    const TypeContext &getTypeContext() const
    {
        return m_ty_ctx;
    }

    TypeContext &getTypeContext()
    {
        return m_ty_ctx;
    }

    Node *buildAdd(Node *lhs, Node *rhs, const Token &tok)
    {
        TypeId lhs_tid = arrayDecay(lhs->type_id);
        TypeId rhs_tid = arrayDecay(rhs->type_id);

        Type l = m_ty_ctx.getType(lhs_tid);
        Type r = m_ty_ctx.getType(rhs_tid);

        if (isInteger(l.kind) && isInteger(r.kind))
        {
            Node *node    = m_arena.alloc<BinaryNode>(NodeKind::ADD, lhs, rhs, tok);
            usualArithConv(lhs, rhs);
            node->type_id = lhs->type_id;
            return node;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
        {
            std::swap(lhs, rhs);
            std::swap(l, r);
            std::swap(lhs_tid, rhs_tid);
        }

        if (r.size < 8)
            rhs = m_arena.alloc<TypeCastNode>(rhs, m_ty_ctx.getLongTypeId(), tok);

        auto scale      = m_arena.alloc<NumNode>(m_ty_ctx.getType(l.base_type_id).size, tok);
        scale->type_id  = m_ty_ctx.getLongTypeId();
        auto factor     = m_arena.alloc<BinaryNode>(NodeKind::MUL, rhs, scale, tok);
        factor->type_id = m_ty_ctx.getLongTypeId();
        auto node       = m_arena.alloc<BinaryNode>(NodeKind::ADD, lhs, factor, tok);
        node->type_id   = lhs_tid;
        return node;
    }

    Node *buildSub(Node *lhs, Node *rhs, const Token &tok)
    {
        TypeId lhs_tid = arrayDecay(lhs->type_id);
        TypeId rhs_tid = arrayDecay(rhs->type_id);

        Type l = m_ty_ctx.getType(lhs_tid);
        Type r = m_ty_ctx.getType(rhs_tid);

        if (isInteger(l.kind) && isInteger(r.kind))
        {
            auto node     = m_arena.alloc<BinaryNode>(NodeKind::SUB, lhs, rhs, tok);
            usualArithConv(lhs, rhs);
            node->type_id = lhs->type_id;
            return node;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
        {
            auto node         = m_arena.alloc<BinaryNode>(NodeKind::SUB, lhs, rhs, tok);
            node->type_id     = m_ty_ctx.getLongTypeId();
            auto scale        = m_arena.alloc<NumNode>(m_ty_ctx.getType(l.base_type_id).size, tok);
            scale->type_id    = m_ty_ctx.getLongTypeId();
            auto div_node     = m_arena.alloc<BinaryNode>(NodeKind::DIV, node, scale, tok);
            div_node->type_id = m_ty_ctx.getLongTypeId();
            return div_node;
        }

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        if (r.size < 8)
            rhs = m_arena.alloc<TypeCastNode>(rhs, m_ty_ctx.getLongTypeId(), tok);

        auto scale      = m_arena.alloc<NumNode>(m_ty_ctx.getType(l.base_type_id).size, tok);
        scale->type_id  = m_ty_ctx.getLongTypeId();
        auto factor     = m_arena.alloc<BinaryNode>(NodeKind::MUL, rhs, scale, tok);
        factor->type_id = m_ty_ctx.getLongTypeId();
        auto node       = m_arena.alloc<BinaryNode>(NodeKind::SUB, lhs, factor, tok);
        node->type_id   = lhs_tid;
        return node;
    }

    Node *buildMul(Node *lhs, Node *rhs, const Token &tok)
    {
        return buildCommonBinary(NodeKind::MUL, lhs, rhs, tok);
    }

    Node *buildDiv(Node *lhs, Node *rhs, const Token &tok)
    {
        return buildCommonBinary(NodeKind::DIV, lhs, rhs, tok);
    }

    Node *buildMod(Node *lhs, Node *rhs, const Token &tok)
    {
        return buildCommonBinary(NodeKind::MOD, lhs, rhs, tok);
    }

    Node *buildAddr(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        auto node = m_arena.alloc<UnaryNode>(NodeKind::ADDR, lhs, tok);
        if (l.kind == TypeKind::ARRAY)
            node->type_id = m_ty_ctx.getPointerTypeId(l.base_type_id);
        else
            node->type_id = m_ty_ctx.getPointerTypeId(lhs->type_id);
        return node;
    }

    Node *buildDeref(Node *lhs, const Token &tok)
    {
        TypeId lhs_tid = arrayDecay(lhs->type_id);
        Type l = m_ty_ctx.getType(lhs->type_id);

        if (l.base_type_id == -1)
            DiagnosticEngine::errorOnTok(tok, "invalid pointer");

        Type l_base = m_ty_ctx.getType(l.base_type_id);
        if (l_base.kind == TypeKind::VOID)
            DiagnosticEngine::errorOnTok(tok, "derefencing a void pointer");

        auto node     = m_arena.alloc<UnaryNode>(NodeKind::DEREF, lhs, tok);
        node->type_id = l.base_type_id;
        return node;
    }

public:
    std::uint32_t getTypeSize(TypeId tid)
    {
        return m_ty_ctx.getType(tid).size;
    }

private:
    Node *buildCommonBinary(NodeKind kind, Node *lhs, Node *rhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (isInteger(l.kind) && isInteger(r.kind))
        {
            Node *node = m_arena.alloc<BinaryNode>(kind, lhs, rhs, tok);
            usualArithConv(lhs, rhs);
            node->type_id = lhs->type_id;
            return node;
        }

        DiagnosticEngine::errorOnTok(tok, "invalid operands");
    }

private:
    TypeId arrayDecay(TypeId tid)
    {
        auto type = m_ty_ctx.getType(tid);

        if (type.kind == TypeKind::ARRAY)
            return m_ty_ctx.getPointerTypeId(type.base_type_id);

        return tid;
    }

    TypeId getCommonTypeId(Node *lhs, Node *rhs)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (l.kind == TypeKind::PTR)
            return m_ty_ctx.getPointerTypeId(l.base_type_id);
        else if (l.size == 8 || r.size == 8)
            return m_ty_ctx.getLongTypeId();
        else
            return m_ty_ctx.getIntTypeId();
    }

    void usualArithConv(Node *&lhs, Node *&rhs)
    {
        auto common_tid = getCommonTypeId(lhs, rhs);
        Type common_type = m_ty_ctx.getType(common_tid);

        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (l.size < common_type.size)
            lhs = m_arena.alloc<TypeCastNode>(lhs, common_tid);

        if (r.size < common_type.size)
            rhs = m_arena.alloc<TypeCastNode>(rhs, common_tid);
    }

    bool isInteger(TypeKind kind)
    {
        return kind == TypeKind::BOOL || kind == TypeKind::CHAR || kind == TypeKind::SHORT || kind == TypeKind::INT || kind == TypeKind::LONG || kind == TypeKind::ENUM;
    }

private:
    TypeContext m_ty_ctx;
    Arena &m_arena;
};