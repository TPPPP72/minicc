#pragma once

#include <AST/Function.hpp>
#include <Diag/Diag.hpp>
#include <Lexer/Lexer.hpp>
#include <charconv>

/// TODO: Here we need a Arena Allocator

class Parser
{
public:
    Parser(TokenViewer token_viewer) : tok(token_viewer), locals(nullptr) {}
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
        if (tok.tryConsumeToken("return"))
        {
            auto node = new Node{NodeType::RETURN, expr()};
            node->tok = tok.prev();
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("if"))
        {
            auto node = new Node{NodeType::IF};
            node->tok = tok.prev();
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
            auto node = new Node{NodeType::FOR};
            node->tok = tok.prev();
            tok.consumeToken("(");
            node->init = exprStmt();
            if (!tok.tryConsumeToken(";")){
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

        if(tok.tryConsumeToken("while")){
            auto node = new Node{NodeType::FOR};
            node->tok = tok.prev();
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

        auto node  = new Node{NodeType::BLOCK};
        node->tok = tok.getToken();
        node->body = head.next;
        return node;
    }

    Node *exprStmt()
    {
        if (tok.tryConsumeToken(";")){
            auto node = new Node{NodeType::BLOCK};
            node->tok = tok.prev();
            return node;
        }

        auto node = new Node{NodeType::EXPR_STMT, expr()};
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        return assign();
    }

    Node *assign()
    {
        auto lhs = equality();

        if (tok.tryConsumeToken("="))
        {
            auto rhs = new Node{NodeType::ASSIGN, lhs, assign()};
            rhs->tok = tok.prev();
            return rhs;
        }

        return lhs;
    }

    Node *equality()
    {
        auto node = relational();

        while (true)
        {
            if (tok.tryConsumeToken("=="))
            {
                node = new Node{NodeType::EQ, node, relational()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                node = new Node{NodeType::NE, node, relational()};
                node->tok = tok.prev();
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
                node = new Node{NodeType::LE, node, add()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                node = new Node{NodeType::LT, node, add()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                node = new Node{NodeType::GE, node, add()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                node = new Node{NodeType::GT, node, add()};
                node->tok = tok.prev();
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
                node = new Node{NodeType::ADD, node, mul()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken("-"))
            {
                node = new Node{NodeType::SUB, node, mul()};
                node->tok = tok.prev();
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
                node = new Node{NodeType::MUL, node, unary()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                node = new Node{NodeType::DIV, node, unary()};
                node->tok = tok.prev();
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                node = new Node{NodeType::MOD, node, unary()};
                node->tok = tok.prev();
                continue;
            }

            return node;
        }
    }

    Node *unary()
    {
        if (tok.tryConsumeToken("+"))
            return unary();

        if (tok.tryConsumeToken("-")){
            auto node = new Node{NodeType::NEG, unary()};
            node->tok = tok.prev();
            return node;
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

        if (token.type == TokenType::IDENT)
        {
            auto var = findVar();
            if (!var)
                var = newLvar();
            tok.skipToken();
            auto node = new Node{var};
            node->tok = tok.prev();
            return node;
        }

        if (token.type == TokenType::NUM)
        {
            std::int32_t value;
            auto content = token.getContent();
            std::from_chars(content.begin(), content.end(), value);
            auto node = new Node{value};
            tok.skipToken();
            node->tok = tok.prev();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    Object *newLvar()
    {
        auto var  = new Object{};
        var->name = tok.getToken().getContent();
        var->next = locals;
        locals    = var;
        return var;
    }

    Object *findVar()
    {
        for (auto var = locals; var; var = var->next)
        {
            if (var->name == tok.getToken().getContent())
                return var;
        }
        return nullptr;
    }

private:
    TokenViewer tok;
    Object *locals;
};