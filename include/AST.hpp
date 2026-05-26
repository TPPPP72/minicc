#pragma once

#include <Diag.hpp>
#include <Lexer.hpp>
#include <charconv>
#include <minicc.hpp>

/// TODO: Here we need a Arena Allocator

class AST
{
public:
    AST(TokenViewer token_viewer) : tok(token_viewer) {}
    Node *parse()
    {
        return compoundStmt();
    }

private:
    Node *compoundStmt()
    {
        if (tok.tryConsumeToken("{"))
        {
            Node head = {};
            Node *cur = &head;
            while (true)
            {
                if (tok.tryConsumeToken("}"))
                    break;
                
                auto token = tok.getToken();
                if (token.type == TokenType::ENDF)
                {
                    DiagnosticEngine::errorOnTok(token, "expected '}}'");
                    break;
                }
                cur = cur->next = stmt();
            }
            return head.next;
        }

        return stmt();
    }

    Node *stmt()
    {
        return expr_stmt();
    }

    Node *expr_stmt()
    {
        auto node = new Node{NodeType::EXPR_STMT, expr()};
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        return add();
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
    TokenViewer tok;
};