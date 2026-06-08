#pragma once

#include <AST/Function.hpp>
#include <Diag/Diag.hpp>
#include <Lexer/Lexer.hpp>
#include <Sema/Sema.hpp>
#include <charconv>

/// TODO: Here we need a Arena Allocator

class Parser
{
public:
    Parser(TokenViewer token_viewer, Sema &sema) : tok(token_viewer), m_sema(sema), locals(nullptr) {}
    Function *parse()
    {
        auto func    = new Function{};
        func->body   = stmt();
        func->locals = locals;
        return func;
    }

private:
    Node *stmt()
    {
        if (tok.getToken().getContent() == "int")
            return declaration();

        if (tok.tryConsumeToken("return"))
        {
            auto kw_tok = tok.prev();
            auto node   = new Node{NodeKind::RETURN, expr()};
            node->tok   = kw_tok;
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("if"))
        {
            auto kw_tok = tok.prev();
            auto node   = new Node{NodeKind::IF};
            node->tok   = kw_tok;
            tok.consumeToken("(");
            node->cond = expr();
            tok.consumeToken(")");
            node->then = stmt();
            if (tok.tryConsumeToken("else"))
                node->els = stmt();
            return node;
        }

        if (tok.tryConsumeToken("for"))
        {
            auto kw_tok = tok.prev();
            auto node   = new Node{NodeKind::FOR};
            node->tok   = kw_tok;
            tok.consumeToken("(");
            node->init = exprStmt();
            if (!tok.tryConsumeToken(";"))
            {
                node->cond = expr();
                tok.consumeToken(";");
            }
            if (!tok.tryConsumeToken(")"))
            {
                node->inc = expr();
                tok.consumeToken(")");
            }
            node->then = stmt();
            return node;
        }

        if (tok.tryConsumeToken("while"))
        {
            auto kw_tok = tok.prev();
            auto node   = new Node{NodeKind::FOR};
            node->tok   = kw_tok;
            tok.consumeToken("(");
            node->cond = expr();
            tok.consumeToken(")");
            node->then = stmt();
            return node;
        }

        if (tok.tryConsumeToken("{"))
            return compoundStmt();

        return exprStmt();
    }

    Node *compoundStmt()
    {
        Node head = {};
        Node *cur = &head;
        while (!tok.tryConsumeToken("}"))
            cur = cur->next = stmt();

        auto node  = new Node{NodeKind::BLOCK};
        node->tok  = tok.getToken();
        node->body = head.next;
        return node;
    }

    Node *declaration()
    {
        TypeId base_tid = declSpec();

        Node head{};
        Node *cur     = &head;
        int var_count = 0;

        while (!tok.tryConsumeToken(";"))
        {
            if (var_count++ > 0)
                tok.consumeToken(",");

            std::string_view var_name;
            Token name_tok;

            TypeId final_tid = declarator(base_tid, var_name, name_tok);

            if (findVarByName(var_name))
                DiagnosticEngine::errorOnTok(name_tok, "redefinition of variable '{}'", var_name);

            Object *var = newLvarWithName(var_name, final_tid);

            if (tok.tryConsumeToken("="))
            {
                auto lhs     = new Node{NodeKind::VAR};
                lhs->var     = var;
                lhs->type_id = final_tid;
                lhs->tok     = name_tok;

                auto rhs = assign();

                auto assign_node     = new Node{NodeKind::ASSIGN, lhs, rhs};
                assign_node->type_id = final_tid;
                assign_node->tok     = name_tok;

                cur = cur->next = new Node{NodeKind::EXPR_STMT, assign_node};
            }
        }

        auto node  = new Node{NodeKind::BLOCK};
        node->tok  = tok.prev();
        node->body = head.next;
        return node;
    }

    Node *exprStmt()
    {
        if (tok.tryConsumeToken(";"))
        {
            auto node = new Node{NodeKind::BLOCK};
            node->tok = tok.prev();
            return node;
        }

        auto node = new Node{NodeKind::EXPR_STMT, expr()};
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        return assign();
    }

    Node *assign()
    {
        auto node = equality();

        if (tok.tryConsumeToken("="))
        {
            auto op_tok   = tok.prev();
            auto rhs      = assign();
            node          = new Node{NodeKind::ASSIGN, node, rhs};
            node->type_id = node->lhs->type_id;
            node->tok     = op_tok;
            if (node->lhs->kind == NodeKind::VAR)
                node->lhs->var->type_id = rhs->type_id;
            return node;
        }

        return node;
    }

    Node *equality()
    {
        auto node = relational();

        while (true)
        {
            if (tok.tryConsumeToken("=="))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::EQ, node, relational()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::NE, node, relational()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            return node;
        }
    }

    Node *relational()
    {
        auto node = add();

        while (true)
        {
            if (tok.tryConsumeToken("<="))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::LE, node, add()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::LT, node, add()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::GE, node, add()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::GT, node, add()};
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                node->tok     = op_tok;
                continue;
            }

            return node;
        }
    }

    Node *add()
    {
        auto node = mul();

        while (true)
        {
            if (tok.tryConsumeToken("+"))
            {
                auto op_tok = tok.prev();
                node        = m_sema.buildAdd(node, mul(), op_tok);
                continue;
            }

            if (tok.tryConsumeToken("-"))
            {
                auto op_tok = tok.prev();
                node        = m_sema.buildSub(node, mul(), op_tok);
                continue;
            }

            return node;
        }
    }

    Node *mul()
    {
        auto node = unary();

        while (true)
        {
            if (tok.tryConsumeToken("*"))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::MUL, node, unary()};
                node->type_id = node->lhs->type_id;
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::DIV, node, unary()};
                node->type_id = node->lhs->type_id;
                node->tok     = op_tok;
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                auto op_tok   = tok.prev();
                node          = new Node{NodeKind::MOD, node, unary()};
                node->type_id = node->lhs->type_id;
                node->tok     = op_tok;
                continue;
            }

            return node;
        }
    }

    Node *unary()
    {
        if (tok.tryConsumeToken("+"))
            return unary();

        if (tok.tryConsumeToken("-"))
        {
            auto op_tok   = tok.prev();
            auto node     = new Node{NodeKind::NEG, unary()};
            node->type_id = node->lhs->type_id;
            node->tok     = op_tok;
            return node;
        }

        if (tok.tryConsumeToken("*"))
        {
            auto op_tok = tok.prev();
            return m_sema.buildDeref(unary(), op_tok);
        }

        if (tok.tryConsumeToken("&"))
        {
            auto op_tok = tok.prev();
            return m_sema.buildAddr(unary(), op_tok);
        }

        return primary();
    }

    Node *primary()
    {
        if (tok.tryConsumeToken("("))
        {
            auto node = expr();
            tok.consumeToken(")");
            return node;
        }

        auto token = tok.getToken();

        if (token.kind == TokenKind::IDENT)
        {
            auto var = findVarByName(token.getContent());
            if (!var)
                DiagnosticEngine::errorOnTok(token, "undeclared identifier '{}'", token.getContent());

            tok.skipToken();
            auto node     = new Node{var};
            node->tok     = tok.prev();
            node->type_id = var->type_id;
            return node;
        }

        if (token.kind == TokenKind::NUM)
        {
            std::int32_t value;
            auto content = token.getContent();
            std::from_chars(content.begin(), content.end(), value);
            auto node = new Node{value};
            tok.skipToken();
            node->tok     = tok.prev();
            node->type_id = m_sema.getTypeContext().getIntTypeId();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    std::string_view getIdent()
    {
        if (tok.getToken().kind != TokenKind::IDENT)
            DiagnosticEngine::errorOnTok(tok.getToken(), "expected an identifier");
        return tok.getToken().getContent();
    }

    TypeId declSpec()
    {
        if (tok.tryConsumeToken("int"))
            return m_sema.getTypeContext().getIntTypeId();
        DiagnosticEngine::errorOnTok(tok.getToken(), "expected a type specifier");
    }

    TypeId declarator(TypeId base_tid, std::string_view &out_name, Token &out_name_tok)
    {
        TypeId current_tid = base_tid;

        while (tok.tryConsumeToken("*"))
            current_tid = m_sema.getTypeContext().getPointerTypeId(current_tid);

        out_name_tok = tok.getToken();
        if (out_name_tok.kind != TokenKind::IDENT)
            DiagnosticEngine::errorOnTok(out_name_tok, "expected a variable name");

        out_name = out_name_tok.getContent();
        tok.skipToken();

        return current_tid;
    }

    Object *newLvarWithName(std::string_view name, TypeId tid)
    {
        auto var     = new Object{};
        var->name    = name;
        var->next    = locals;
        var->type_id = tid;
        locals       = var;
        return var;
    }

    Object *findVarByName(std::string_view name)
    {
        for (auto var = locals; var; var = var->next)
        {
            if (var->name == name)
                return var;
        }
        return nullptr;
    }

private:
    TokenViewer tok;
    Sema &m_sema;
    Object *locals;
};