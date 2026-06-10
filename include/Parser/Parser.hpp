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
    Parser(TokenViewer token_viewer, Sema &sema) : tok(token_viewer), m_sema(sema) {}
    std::vector<Function *> parseProgram()
    {
        std::vector<Function *> program;

        while (tok.getToken().kind != TokenKind::ENDF)
        {
            m_locals.clear();
            program.push_back(parseFunction());
        }

        return program;
    }

private:
    Function *parseFunction()
    {
        TypeId ret_tid = declSpec();

        std::string_view func_name = getIdent();
        Token name_tok             = tok.getToken();
        tok.skipToken();

        tok.skipToken();

        auto func    = new Function{};
        func->name   = func_name;

        std::uint32_t param_count{};

        while (!tok.tryConsumeToken(")"))
        {
            if (param_count > 0)
                tok.consumeToken(",");

            TypeId p_base = declSpec();

            std::string_view p_name = getIdent();
            tok.skipToken();

            Object *param_var = newLvarWithName(p_name, p_base);

            func->params.push_back(param_var);
            param_count++;
        }

        tok.consumeToken("{");
        func->body   = compoundStmt();
        func->locals = m_locals;

        return func;
    }

    Node *stmt()
    {
        if (tok.getToken().getContent() == "int")
            return declaration();

        if (tok.tryConsumeToken("return"))
        {
            auto kw_tok = tok.prev();
            auto node   = new ReturnNode(expr(), kw_tok);
            node->tok   = kw_tok;
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("if"))
        {
            auto kw_tok = tok.prev();
            auto node   = new IfNode(kw_tok);
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
            auto node   = new ForNode{kw_tok};
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
            auto node   = new ForNode{kw_tok};
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
        auto node = new BlockNode(tok.getToken());

        while (!tok.tryConsumeToken("}"))
        {
            if (tok.getToken().getContent() == "int")
                node->stmts.push_back(declaration());
            else
                node->stmts.push_back(stmt());
        }
        return node;
    }

    Node *declaration()
    {
        TypeId base_tid = declSpec();

        auto node     = new BlockNode(tok.getToken());
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
                auto lhs     = new VarNode{var, name_tok};
                lhs->type_id = final_tid;

                auto rhs = assign();

                auto assign_node     = new BinaryNode(NodeKind::ASSIGN, lhs, rhs, name_tok);
                assign_node->type_id = final_tid;

                node->stmts.push_back(new ExprStmtNode(assign_node, name_tok));
            }
        }

        node->tok = tok.prev();
        return node;
    }

    Node *exprStmt()
    {
        if (tok.tryConsumeToken(";"))
            return new BlockNode(tok.prev());

        auto node = new ExprStmtNode(expr(), tok.getToken());
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        return assign();
    }

    Node *funCall()
    {
        auto identifier = tok.getToken();

        auto node     = new FuncCallNode{identifier.getContent(), identifier};
        node->type_id = m_sema.getTypeContext().getIntTypeId();

        tok.skipToken();
        tok.skipToken();

        bool is_first_arg{true};
        while (!tok.tryConsumeToken(")"))
        {
            if (!is_first_arg)
                tok.consumeToken(",");

            node->args.emplace_back(assign());
            is_first_arg = false;
        }

        return node;
    }

    Node *assign()
    {
        auto node = equality();

        if (tok.tryConsumeToken("="))
        {
            auto op_tok          = tok.prev();
            auto rhs             = assign();
            auto assign_node     = new BinaryNode(NodeKind::ASSIGN, node, rhs, op_tok);
            assign_node->type_id = node->type_id;
            if (node->kind == NodeKind::VAR)
            {
                auto var_node          = static_cast<VarNode *>(node);
                var_node->var->type_id = rhs->type_id;
            }
            return assign_node;
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
                auto rhs      = relational();
                node          = new BinaryNode(NodeKind::EQ, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                auto op_tok   = tok.prev();
                auto rhs      = relational();
                node          = new BinaryNode(NodeKind::NE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
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
                auto rhs      = add();
                node          = new BinaryNode(NodeKind::LE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = new BinaryNode(NodeKind::LT, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = new BinaryNode(NodeKind::GE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = new BinaryNode(NodeKind::GT, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
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
                auto rhs      = unary();
                TypeId ty     = node->type_id;
                node          = new BinaryNode(NodeKind::MUL, node, rhs, op_tok);
                node->type_id = ty;
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = unary();
                TypeId ty     = node->type_id;
                node          = new BinaryNode(NodeKind::DIV, node, rhs, op_tok);
                node->type_id = ty;
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = unary();
                TypeId ty     = node->type_id;
                node          = new BinaryNode(NodeKind::MOD, node, rhs, op_tok);
                node->type_id = ty;
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
            auto node     = new UnaryNode(NodeKind::NEG, unary(), op_tok);
            node->type_id = node->lhs->type_id;
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
            if (tok.next().getContent() == "(")
                return funCall();

            auto var = findVarByName(token.getContent());
            if (!var)
                DiagnosticEngine::errorOnTok(token, "undeclared identifier '{}'", token.getContent());

            tok.skipToken();
            auto node     = new VarNode{var, tok.prev()};
            node->type_id = var->type_id;
            return node;
        }

        if (token.kind == TokenKind::NUM)
        {
            std::int32_t value;
            auto content = token.getContent();
            std::from_chars(content.begin(), content.end(), value);
            auto node = new NumNode{value, tok.getToken()};
            tok.skipToken();
            node->type_id = m_sema.getTypeContext().getIntTypeId();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    std::string_view getIdent()
    {
        if (tok.getToken().kind != TokenKind::IDENT && tok.getToken().kind != TokenKind::KEYWORD)
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
        var->type_id = tid;
        m_locals.push_back(std::move(var));
        return m_locals.back();
    }

    Object *findVarByName(std::string_view name)
    {
        for (auto var : m_locals)
        {
            if (var->name == name)
                return var;
        }
        return nullptr;
    }

private:
    TokenViewer tok;
    Sema &m_sema;
    std::vector<Object *> m_locals;
};