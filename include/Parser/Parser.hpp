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
            program.push_back(funcDecl());
        }

        return program;
    }

private:
    Function *funcDecl()
    {
        auto ret_tid = declSpec();

        Token ident;
        auto func_tid = declarator(ret_tid, ident);

        auto func_sign = m_sema.getTypeContext().getFuncSignature(func_tid);

        auto func  = new Function{};
        func->name = ident.getContent();

        for (size_t i = 0; i < func_sign.param_types.size(); ++i)
            func->params.push_back(newLvarWithName(func_sign.param_names[i], func_sign.param_types[i]));

        tok.consumeToken("{");
        func->body   = compoundStmt();
        func->locals = m_locals;

        return func;
    }

    Node *varDecl()
    {
        TypeId base_tid = declSpec();

        auto node     = new BlockNode(tok.getToken());
        int var_count = 0;

        while (!tok.tryConsumeToken(";"))
        {
            if (var_count++ > 0)
                tok.consumeToken(",");

            Token ident;

            TypeId final_tid = declarator(base_tid, ident);

            if (findVarByName(ident.getContent()))
                DiagnosticEngine::errorOnTok(ident, "redefinition of variable '{}'", ident.getContent());

            Object *var = newLvarWithName(ident.getContent(), final_tid);

            if (tok.tryConsumeToken("="))
            {
                auto lhs     = new VarNode{var, ident};
                lhs->type_id = final_tid;

                auto rhs = assign();

                auto assign_node     = new BinaryNode(NodeKind::ASSIGN, lhs, rhs, ident);
                assign_node->type_id = final_tid;

                node->stmts.push_back(new ExprStmtNode(assign_node, ident));
            }
        }

        node->tok = tok.prev();
        return node;
    }

private:
    Node *stmt()
    {
        if (tok.getToken().getContent() == "int")
            return varDecl();

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
                node->stmts.push_back(varDecl());
            else
                node->stmts.push_back(stmt());
        }
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

    Node *assign()
    {
        auto node = equality();

        if (tok.tryConsumeToken("="))
        {
            auto op_tok          = tok.prev();
            auto rhs             = assign();
            auto assign_node     = new BinaryNode(NodeKind::ASSIGN, node, rhs, op_tok);
            assign_node->type_id = node->type_id;
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

        return postfix();
    }

    Node *postfix()
    {
        auto node = primary();

        while (tok.tryConsumeToken("["))
        {
            auto op_tok = tok.prev();
            auto idx    = expr();
            tok.consumeToken("]");

            auto offset = m_sema.buildAdd(node, idx, op_tok);
            node          = m_sema.buildDeref(offset, op_tok);
        }

        return node;
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
            tok.skipToken();

            if (tok.tryConsumeToken("("))
                return funCall(token);

            auto var = findVarByName(token.getContent());
            if (!var)
                DiagnosticEngine::errorOnTok(token, "undeclared identifier '{}'", token.getContent());

            auto node     = new VarNode{var, token};
            node->type_id = var->type_id;
            return node;
        }

        if (token.kind == TokenKind::NUM)
        {
            auto node = new NumNode{getNumber(), tok.getToken()};
            tok.skipToken();
            node->type_id = m_sema.getTypeContext().getIntTypeId();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    TypeId declSpec()
    {
        if (tok.tryConsumeToken("int"))
            return m_sema.getTypeContext().getIntTypeId();

        DiagnosticEngine::errorOnTok(tok.getToken(), "expected a type specifier");
    }

    TypeId declarator(TypeId base_tid, Token &ident)
    {
        TypeId current_tid = base_tid;

        while (tok.tryConsumeToken("*"))
            current_tid = m_sema.getTypeContext().getPointerTypeId(current_tid);

        ident = getIdentToken();
        tok.skipToken();

        current_tid = declSuffix(current_tid);

        return current_tid;
    }

    TypeId declFuncParams(TypeId ret_tid)
    {
        std::vector<TypeId> param_types;
        std::vector<std::string_view> param_names;

        while (!tok.tryConsumeToken(")"))
        {
            if (!param_types.empty())
                tok.consumeToken(",");

            auto base_tid = declSpec();
            Token ident;
            auto tid = declarator(base_tid, ident);

            param_types.emplace_back(tid);
            param_names.emplace_back(ident.getContent());
        }

        return m_sema.getTypeContext().getFunctionTypeId(ret_tid, param_types, param_names);
    }

    TypeId declSuffix(TypeId base_tid)
    {
        if (tok.tryConsumeToken("("))
            return declFuncParams(base_tid);

        if (tok.tryConsumeToken("["))
        {
            auto len = getNumber();
            tok.skipToken();
            tok.consumeToken("]");
            base_tid = declSuffix(base_tid);
            return m_sema.getTypeContext().getArrayTypeId(base_tid, len);
        }

        return base_tid;
    }

private:
    Node *funCall(const Token &ident)
    {
        auto node     = new FuncCallNode{ident.getContent(), ident};
        node->type_id = m_sema.getTypeContext().getIntTypeId();

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

    Token getIdentToken()
    {
        if (tok.getToken().kind != TokenKind::IDENT && tok.getToken().kind != TokenKind::KEYWORD)
            DiagnosticEngine::errorOnTok(tok.getToken(), "expected an identifier");

        return tok.getToken();
    }

    std::int32_t getNumber()
    {
        if (tok.getToken().kind != TokenKind::NUM)
            DiagnosticEngine::errorOnTok(tok.getToken(), "expected a number");

        std::int32_t value;
        auto content = tok.getToken().getContent();
        std::from_chars(content.begin(), content.end(), value);
        return value;
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