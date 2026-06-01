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
        tok.consumeToken("{");
        auto func    = new Function{};
        func->body   = compoundStmt();
        func->locals = locals;
        return func;
    }

private:
    Node *compoundStmt()
    {
        Node head = {};
        Node *cur = &head;
        while (!tok.tryConsumeToken("}"))
            cur = cur->next = stmt();

        auto node  = new Node{NodeType::BLOCK};
        node->body = head.next;
        return node;
    }

    Node *stmt()
    {
        if (tok.tryConsumeToken("return"))
        {
            auto node = new Node{NodeType::RETURN, expr()};
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("{"))
            return compoundStmt();

        return exprStmt();
    }

    Node *exprStmt()
    {
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
        auto node = equality();

        if (tok.tryConsumeToken("="))
            return new Node{NodeType::ASSIGN, node, assign()};

        return node;
    }

    Node *equality()
    {
        auto node = relational();

        while (true)
        {
            if (tok.tryConsumeToken("=="))
            {
                node = new Node{NodeType::EQ, node, relational()};
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                node = new Node{NodeType::NE, node, relational()};
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
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                node = new Node{NodeType::LT, node, add()};
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                node = new Node{NodeType::GE, node, add()};
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                node = new Node{NodeType::GT, node, add()};
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
                continue;
            }

            if (tok.tryConsumeToken("-"))
            {
                node = new Node{NodeType::SUB, node, mul()};
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
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                node = new Node{NodeType::DIV, node, unary()};
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                node = new Node{NodeType::MOD, node, unary()};
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
            return new Node{NodeType::NEG, unary()};

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
            return new Node{var};
        }

        if (token.type == TokenType::NUM)
        {
            std::int32_t value;
            auto content = token.getContent();
            std::from_chars(content.begin(), content.end(), value);
            auto node = new Node{value};
            tok.skipToken();
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