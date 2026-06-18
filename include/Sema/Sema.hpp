#pragma once

#include <AST/Node.hpp>
#include <AST/Type.hpp>
#include <Diag/Diag.hpp>
#include <Sema/TypeContext.hpp>

class Sema
{
public:
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
            Node *node    = new BinaryNode{NodeKind::ADD, lhs, rhs, tok};
            node->type_id = m_ty_ctx.getIntTypeId();
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

        auto scale      = new NumNode(m_ty_ctx.getType(l.base_type_id).size);
        scale->type_id  = m_ty_ctx.getIntTypeId();
        auto factor     = new BinaryNode{NodeKind::MUL, rhs, scale};
        factor->type_id = m_ty_ctx.getIntTypeId();
        auto node       = new BinaryNode{NodeKind::ADD, lhs, factor, tok};
        node->type_id   = lhs->type_id;
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
            auto result     = new BinaryNode(NodeKind::SUB, lhs, rhs, tok);
            result->type_id = m_ty_ctx.getIntTypeId();
            return result;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
        {
            auto node         = new BinaryNode{NodeKind::SUB, lhs, rhs, tok};
            node->type_id     = m_ty_ctx.getIntTypeId();
            auto scale        = new NumNode(m_ty_ctx.getType(l.base_type_id).size);
            scale->type_id    = m_ty_ctx.getIntTypeId();
            auto div_node     = new BinaryNode{NodeKind::DIV, node, scale};
            div_node->type_id = m_ty_ctx.getIntTypeId();
            return div_node;
        }

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto scale      = new NumNode(m_ty_ctx.getType(l.base_type_id).size);
        scale->type_id  = m_ty_ctx.getIntTypeId();
        auto factor     = new BinaryNode{NodeKind::MUL, rhs, scale};
        factor->type_id = m_ty_ctx.getIntTypeId();
        auto node       = new BinaryNode{NodeKind::SUB, lhs, factor, tok};
        node->type_id   = lhs->type_id;
        return node;
    }

    Node *buildAddr(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        auto node = new UnaryNode{NodeKind::ADDR, lhs, tok};
        if (l.kind == TypeKind::ARRAY)
            node->type_id = m_ty_ctx.getPointerTypeId(l.base_type_id);
        else
            node->type_id = m_ty_ctx.getPointerTypeId(lhs->type_id);
        return node;
    }

    Node *buildDeref(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        if (l.base_type_id == 0)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto node     = new UnaryNode{NodeKind::DEREF, lhs, tok};
        node->type_id = m_ty_ctx.getType(lhs->type_id).base_type_id;
        return node;
    }

public:
    std::uint32_t getTypeSize(TypeId tid){
        return m_ty_ctx.getType(tid).size;
    }

private:
    TypeId arrayDecay(TypeId tid)
    {
        auto type = m_ty_ctx.getType(tid);

        if (type.kind == TypeKind::ARRAY)
            return m_ty_ctx.getPointerTypeId(type.base_type_id);

        return tid;
    }

    bool isInteger(TypeKind kind){
        return kind == TypeKind::INT || kind == TypeKind::CHAR;
    }

private:
    TypeContext m_ty_ctx;
};