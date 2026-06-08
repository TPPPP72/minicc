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
            Node *node    = new Node(NodeKind::ADD, lhs, rhs);
            node->type_id = m_ty_ctx.getIntTypeId();
            node->tok     = tok;
            return node;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            std::swap(lhs, rhs);

        auto scale      = new Node{8};
        scale->type_id  = m_ty_ctx.getIntTypeId();
        auto factor     = new Node{NodeKind::MUL, rhs, scale};
        factor->type_id = m_ty_ctx.getIntTypeId();
        auto node       = new Node{NodeKind::ADD, lhs, factor};
        node->type_id   = lhs->type_id;
        node->tok       = tok;
        return node;
    }

    Node *buildSub(Node *lhs, Node *rhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);
        Type r = m_ty_ctx.getType(rhs->type_id);

        if (l.kind == TypeKind::INT && r.kind == TypeKind::INT)
        {
            auto result     = new Node(NodeKind::SUB, lhs, rhs);
            result->type_id = m_ty_ctx.getIntTypeId();
            result->tok     = tok;
            return result;
        }

        if (l.kind == TypeKind::PTR && r.kind == TypeKind::PTR)
        {
            auto node         = new Node{NodeKind::SUB, lhs, rhs};
            node->type_id     = m_ty_ctx.getIntTypeId();
            node->tok         = tok;
            auto scale        = new Node{8};
            scale->type_id    = m_ty_ctx.getIntTypeId();
            auto div_node     = new Node{NodeKind::DIV, node, scale};
            div_node->type_id = m_ty_ctx.getIntTypeId();
            return div_node;
        }

        if (l.kind == TypeKind::INT && r.kind == TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto scale      = new Node{8};
        scale->type_id  = m_ty_ctx.getIntTypeId();
        auto factor     = new Node{NodeKind::MUL, rhs, scale};
        factor->type_id = m_ty_ctx.getIntTypeId();
        auto node       = new Node{NodeKind::SUB, lhs, factor};
        node->type_id   = lhs->type_id;
        node->tok       = tok;
        return node;
    }

    Node *buildAddr(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        auto node     = new Node{NodeKind::ADDR, lhs};
        node->type_id = m_ty_ctx.getPointerTypeId(lhs->type_id);
        node->tok     = tok;
        return node;
    }

    Node *buildDeref(Node *lhs, const Token &tok)
    {
        Type l = m_ty_ctx.getType(lhs->type_id);

        if (l.kind != TypeKind::PTR)
            DiagnosticEngine::errorOnTok(tok, "invalid operands");

        auto node     = new Node{NodeKind::DEREF, lhs};
        node->type_id = m_ty_ctx.getType(lhs->type_id).base_type_id;
        node->tok     = tok;
        return node;
    }

private:
    TypeContext m_ty_ctx;
};