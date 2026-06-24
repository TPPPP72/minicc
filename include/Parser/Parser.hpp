#pragma once

#include <Diag/Diag.hpp>
#include <Lexer/Lexer.hpp>
#include <Scope/Function.hpp>
#include <Scope/SymbolTable.hpp>
#include <Scope/Variable.hpp>
#include <Sema/Sema.hpp>
#include <Util/Tags.hpp>
#include <charconv>
#include <cstring>

class Parser
{
public:
    Parser(TokenViewer token_viewer, Sema &sema, Arena &arena) : m_sym_table(arena), tok(token_viewer), m_sema(sema), m_arena(arena) {}

    std::vector<Symbol *> parseProgram()
    {
        while (tok.getToken().kind != TokenKind::ENDF)
        {
            if (isFunction())
                funcDecl();
            else
                varDecl<IsGlobal>();
        }
        return m_sym_table.getGlobalSymbols();
    }

private:
    template<typename T>
    TypeId structAndUnionDecl()
    {
        std::string_view tag;
        auto &token = tok.getToken();
        if (token.kind == TokenKind::IDENT)
        {
            tag = token.getContent();
            tok.skipToken();
        }
        if (!tok.isToken("{"))
        {
            auto layout_tid = m_sym_table.lookupTag(tag);
            if (layout_tid == -1)
            {
                if constexpr (std::is_same_v<T, IsStruct>)
                    DiagnosticEngine::errorOnTok(token, "struct undefined");
                else
                    DiagnosticEngine::errorOnTok(token, "union undefined");
            }
            return layout_tid;
        }

        tok.skipToken(); // must be '{'
        std::vector<std::pair<std::string_view, TypeId>> members;
        while (!tok.tryConsumeToken("}"))
        {
            auto basety = declSpec();
            int count   = 0;
            while (!tok.tryConsumeToken(";"))
            {
                if (count++ > 0)
                    tok.consumeToken(",");

                Token ident;
                auto type_id = declarator(basety, ident);
                members.emplace_back(ident.getContent(), type_id);
            }
        }

        TypeId layout_tid;

        if constexpr (std::is_same_v<T, IsStruct>)
            layout_tid = m_sema.getTypeContext().getStructTypeId(members);
        else
            layout_tid = m_sema.getTypeContext().getUnionTypeId(members);

        if (!tag.empty())
            m_sym_table.insertTag(tag, layout_tid);
        return layout_tid;
    }

    void funcDecl()
    {
        auto basety = declSpec();

        Token ident;
        auto func_tid = declarator(basety, ident);

        auto func_sign = m_sema.getTypeContext().getFuncSignature(func_tid);

        auto func  = m_arena.alloc<Function>();
        func->name = ident.getContent();

        m_sym_table.insertGlobalIdent(func->name, func);

        func->is_definition = !tok.tryConsumeToken(";");
        if (!func->is_definition)
            return;

        m_sym_table.enterScope();

        for (size_t i = 0; i < func_sign.param_types.size(); ++i)
            func->params.push_back(newVar<IsLocal>(func_sign.param_names[i], func_sign.param_types[i]));

        tok.consumeToken("{");
        func->body   = compoundStmt();
        func->locals = m_sym_table.collectLocalsFromCurrentTree();

        m_sym_table.leaveScope();
    }

    template <typename T>
    Node *varDecl()
    {
        TypeId basety = declSpec();

        auto node     = m_arena.alloc<BlockNode>(tok.getToken());
        int var_count = 0;

        while (!tok.tryConsumeToken(";"))
        {
            if (var_count++ > 0)
                tok.consumeToken(",");

            Token ident;

            TypeId final_tid = declarator(basety, ident);

            auto var = newVar<T>(ident.getContent(), final_tid);

            if (tok.tryConsumeToken("="))
            {
                if constexpr (std::is_same_v<T, IsGlobal>)
                {
                    var->has_int_init = true;
                    var->int_init_val = getNumber();
                    tok.skipToken();
                }
                else
                {
                    auto lhs     = m_arena.alloc<VarNode>(var, ident);
                    lhs->type_id = final_tid;

                    auto rhs = assign();

                    auto assign_node     = m_arena.alloc<BinaryNode>(NodeKind::ASSIGN, lhs, rhs, ident);
                    assign_node->type_id = final_tid;

                    node->stmts.push_back(m_arena.alloc<ExprStmtNode>(assign_node, ident));
                }
            }
        }

        node->tok = tok.prev();
        return node;
    }

private:
    Node *stmt()
    {
        if (isTypename())
            return varDecl<IsLocal>();

        if (tok.tryConsumeToken("return"))
        {
            auto kw_tok = tok.prev();
            auto node   = m_arena.alloc<ReturnNode>(expr(), kw_tok);
            node->tok   = kw_tok;
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("if"))
        {
            auto kw_tok = tok.prev();
            auto node   = m_arena.alloc<IfNode>(kw_tok);
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
            auto node   = m_arena.alloc<ForNode>(kw_tok);
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
            auto node   = m_arena.alloc<ForNode>(kw_tok);
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
        auto node = m_arena.alloc<BlockNode>(tok.getToken());

        m_sym_table.enterScope();

        while (!tok.tryConsumeToken("}"))
        {
            if (tok.getToken().getContent() == "int")
                node->stmts.push_back(varDecl<IsLocal>());
            else
                node->stmts.push_back(stmt());
        }

        m_sym_table.leaveScope();
        return node;
    }

    Node *exprStmt()
    {
        if (tok.tryConsumeToken(";"))
            return m_arena.alloc<BlockNode>(tok.prev());

        auto node = m_arena.alloc<ExprStmtNode>(expr(), tok.getToken());
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        auto node =  assign();

        while (tok.tryConsumeToken(","))
        {
            auto op_tok = tok.prev();
            auto rhs    = assign();
            node          = m_arena.alloc<BinaryNode>(NodeKind::COMMA, node, rhs, op_tok);
            node->type_id = rhs->type_id;
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
            auto assign_node     = m_arena.alloc<BinaryNode>(NodeKind::ASSIGN, node, rhs, op_tok);
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
                node          = m_arena.alloc<BinaryNode>(NodeKind::EQ, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                auto op_tok   = tok.prev();
                auto rhs      = relational();
                node          = m_arena.alloc<BinaryNode>(NodeKind::NE, node, rhs, op_tok);
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
                node          = m_arena.alloc<BinaryNode>(NodeKind::LE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::LT, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::GE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::GT, node, rhs, op_tok);
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
                node          = m_arena.alloc<BinaryNode>(NodeKind::MUL, node, rhs, op_tok);
                node->type_id = ty;
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = unary();
                TypeId ty     = node->type_id;
                node          = m_arena.alloc<BinaryNode>(NodeKind::DIV, node, rhs, op_tok);
                node->type_id = ty;
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                auto op_tok   = tok.prev();
                auto rhs      = unary();
                TypeId ty     = node->type_id;
                node          = m_arena.alloc<BinaryNode>(NodeKind::MOD, node, rhs, op_tok);
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
            auto node     = m_arena.alloc<UnaryNode>(NodeKind::NEG, unary(), op_tok);
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

        if (tok.tryConsumeToken("sizeof"))
        {
            auto op_tok       = tok.prev();
            auto node         = unary();
            auto size         = m_sema.getTypeSize(node->type_id);
            auto num_node     = m_arena.alloc<NumNode>(size, op_tok);
            num_node->type_id = m_sema.getTypeContext().getIntTypeId();
            return num_node;
        }

        return postfix();
    }

    Node *postfix()
    {
        auto node = primary();

        while (true)
        {
            if (tok.tryConsumeToken("["))
            {
                auto op_tok = tok.prev();
                auto idx    = expr();
                tok.consumeToken("]");

                auto offset = m_sema.buildAdd(node, idx, op_tok);
                node        = m_sema.buildDeref(offset, op_tok);
                continue;
            }

            if (tok.tryConsumeToken("."))
            {
                node = structRef(node);
                continue;
            }

            if (tok.tryConsumeToken("->"))
            {
                node = m_sema.buildDeref(node, tok.prev());
                node = structRef(node);
                continue;
            }

            return node;
        }
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

            auto var = static_cast<Variable *>(m_sym_table.lookupIdent(token.getContent()));
            if (!var)
                DiagnosticEngine::errorOnTok(token, "undeclared identifier '{}'", token.getContent());

            auto node     = m_arena.alloc<VarNode>(var, token);
            node->type_id = var->type_id;
            return node;
        }

        if (token.kind == TokenKind::STR)
        {
            auto &ty_context   = m_sema.getTypeContext();
            auto str_array_tid = ty_context.getArrayTypeId(ty_context.getCharTypeId(), token.string_val.length() + 1);

            auto var               = newAnonGvar(str_array_tid);
            var->is_string_literal = true;
            var->string_data       = std::move(token.string_val);
            tok.skipToken();

            auto node     = m_arena.alloc<VarNode>(var, token);
            node->type_id = str_array_tid;
            return node;
        }

        if (token.kind == TokenKind::NUM)
        {
            auto node = m_arena.alloc<NumNode>(getNumber(), tok.getToken());
            tok.skipToken();
            node->type_id = m_sema.getTypeContext().getIntTypeId();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    TypeId declSpec()
    {
        if (tok.tryConsumeToken("char"))
            return m_sema.getTypeContext().getCharTypeId();

        if (tok.tryConsumeToken("short"))
            return m_sema.getTypeContext().getShortTypeId();

        if (tok.tryConsumeToken("int"))
            return m_sema.getTypeContext().getIntTypeId();

        if (tok.tryConsumeToken("long"))
            return m_sema.getTypeContext().getLongTypeId();

        if(tok.tryConsumeToken("struct"))
            return structAndUnionDecl<IsStruct>();

        if(tok.tryConsumeToken("union"))
            return structAndUnionDecl<IsUnion>();

        DiagnosticEngine::errorOnTok(tok.getToken(), "expected a type specifier");
    }

    TypeId declarator(TypeId base_tid, Token &ident)
    {
        TypeId current_tid = base_tid;

        while (tok.tryConsumeToken("*"))
            current_tid = m_sema.getTypeContext().getPointerTypeId(current_tid);

        if (tok.tryConsumeToken("("))
        {
            auto begin_tok = tok;

            Token dummy_ident;
            declarator(base_tid, dummy_ident);
            tok.consumeToken(")");

            current_tid = declSuffix(current_tid);

            RAIITokReverter rvt(tok);
            tok = begin_tok;
            return declarator(current_tid, ident);
        }

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
        auto node     = m_arena.alloc<FuncCallNode>(ident.getContent(), ident);
        node->type_id = m_sema.getTypeContext().getLongTypeId();

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

    Node *structRef(Node *lhs)
    {
        auto kind = m_sema.getTypeContext().getType(lhs->type_id).kind;
        if (kind != TypeKind::STRUCT && kind != TypeKind::UNION)
            DiagnosticEngine::errorOnTok(lhs->tok, "not a struct or union");

        auto &token = tok.getToken();

        auto &struct_layout = m_sema.getTypeContext().getLayout(lhs->type_id);
        auto &members       = struct_layout.members;
        auto it             = std::find_if(members.begin(), members.end(), [&token](const Member &mem)
                                           {
                                   return mem.name == token.getContent();
                                           });

        if (it == members.end())
            DiagnosticEngine::errorOnTok(token, "no member '{}'", token.getContent());

        auto node     = m_arena.alloc<MemberNode>(lhs, it->offset, token);
        node->type_id = it->type_id;
        tok.skipToken();
        return node;
    }

    Token getIdentToken()
    {
        if (tok.getToken().kind != TokenKind::IDENT && tok.getToken().getContent() != "main")
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

    bool isTypename()
    {
        auto token = tok.getToken().getContent();
        return (token == "char" || token == "short" || token == "int" || token == "long" || token == "struct" || token == "union");
    }

    bool isFunction()
    {
        if (tok.isToken(";"))
            return false;

        RAIITokReverter rvt(tok);
        auto basety = declSpec();
        Token ident;
        auto type_id = declarator(basety, ident);
        return m_sema.getTypeContext().getType(type_id).kind == TypeKind::FUNCTION;
    }

    template <typename T>
    Variable *newVar(std::string_view name, TypeId tid)
    {
        auto var     = m_arena.alloc<Variable>();
        var->name    = name;
        var->type_id = tid;
        if constexpr (std::is_same_v<T, IsGlobal>)
        {
            m_sym_table.insertGlobalIdent(name, var);
        }
        else
        {
            var->is_local = true;
            m_sym_table.registerLocal(var);
            if (!m_sym_table.insertIdent(name, var))
                DiagnosticEngine::errorOnTok(tok.getToken(), "redefinition of variable '{}'", name);
        }
        return var;
    }

    std::string_view newUniqueName()
    {
        static int id = 0;
        std::string name_str = ".L.." + std::to_string(id++);
        
        auto buf = m_arena.alloc<char>(name_str.size());
        std::memcpy(buf, name_str.data(), name_str.size());
        
        return std::string_view(buf, name_str.size());
    }

    Variable *newAnonGvar(TypeId tid)
    {
        std::string_view safe_name = newUniqueName();
        return newVar<IsGlobal>(safe_name, tid);
    }

private:
    SymbolTable m_sym_table;
    TokenViewer tok;
    Sema &m_sema;
    Arena &m_arena;
};