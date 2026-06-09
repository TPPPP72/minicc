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
        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (l.kind == TypeKind::INT && r.kind == TypeKind::INT)
        {
            Node *node    = new BinaryNode{NodeKind::ADD, lhs, rhs, tok};
            node->type_id = m_ty_ctx.getIntTypeId();
            return node;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            std::swap(lhs, rhs);

        auto scale      = new NumNode{8};
        scale->type_id  = m_ty_ctx.getIntTypeId();
        auto factor     = new BinaryNode{NodeKind::MUL, rhs, scale};
        factor->type_id = m_ty_ctx.getIntTypeId();
        auto node       = new BinaryNode{NodeKind::ADD, lhs, factor, tok};
        node->type_id   = lhs->type_id;
        return node;
    }

    Node *buildSub(Node *lhs, Node *rhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (l.kind == TypeKind::INT && r.kind == TypeKind::INT)
        {
            auto result     = new BinaryNode(NodeKind::SUB, lhs, rhs, tok);
            result->type_id = m_ty_ctx.getIntTypeId();
            return result;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
        {
            auto node         = new BinaryNode{NodeKind::SUB, lhs, rhs, tok};
            node->type_id     = m_ty_ctx.getIntTypeId();
            auto scale        = new NumNode{8};
            scale->type_id    = m_ty_ctx.getIntTypeId();
            auto div_node     = new BinaryNode{NodeKind::DIV, node, scale};
            div_node->type_id = m_ty_ctx.getIntTypeId();
            return div_node;
        }

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto scale      = new NumNode{8};
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

        auto node     = new UnaryNode{NodeKind::ADDR, lhs, tok};
        node->type_id = m_ty_ctx.getPointerTypeId(lhs->type_id);
        return node;
    }

    Node *buildDeref(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        if (l.kind != TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto node     = new UnaryNode{NodeKind::DEREF, lhs, tok};
        node->type_id = m_ty_ctx.getType(lhs->type_id).base_type_id;
        return node;
    }

private:
    TypeContext m_ty_ctx;
};