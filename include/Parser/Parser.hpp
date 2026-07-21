#pragma once

#include <Attr/Attr.hpp>
#include <Diag/Diag.hpp>
#include <Lexer/Lexer.hpp>
#include <Scope/Enum.hpp>
#include <Scope/Function.hpp>
#include <Scope/SymbolTable.hpp>
#include <Scope/Typedef.hpp>
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
            uint32_t attr{};
            if (isAttr())
            {
                if (tok.tryConsumeToken("typedef"))
                    attr += TYPEDEF;
                else if (tok.tryConsumeToken("static"))
                    attr += STATIC;
            }

            if (isFunction())
                funcDecl(attr);
            else if (isTypename())
                varDecl<IsGlobal>(attr);
        }
        return m_sym_table.getGlobalSymbols();
    }

private:
    template <typename T>
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

    TypeId enumDecl()
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
            auto info_tid = m_sym_table.lookupTag(tag);
            if (info_tid == -1)
                DiagnosticEngine::errorOnTok(token, "enum undefined");
            return info_tid;
        }
        tok.skipToken(); // must be '{'

        std::vector<std::pair<std::string_view, int64_t>> members;
        int count = 0;
        int value = 0;
        while (!tok.tryConsumeToken("}"))
        {
            if (count++ > 0)
                tok.consumeToken(",");

            auto &ident = tok.getToken();
            tok.skipToken();

            if (tok.tryConsumeToken("="))
            {
                value = getNumber();
                tok.skipToken();
            }

            auto *e    = m_arena.alloc<Enum>();
            e->name    = ident.getContent();
            e->type_id = m_sema.getTypeContext().getIntTypeId();
            e->val     = value;

            m_sym_table.insertIdent(ident.getContent(), e);
            members.emplace_back(ident.getContent(), value);

            ++value;
        }
        auto info_tid = m_sema.getTypeContext().getEnumTypeId(members);

        if (!tag.empty())
            m_sym_table.insertTag(tag, info_tid);

        return info_tid;
    }

    void funcDecl(uint32_t attr)
    {
        auto basety = declSpec();

        Token ident;
        auto func_tid = declarator(basety, ident);

        auto func_sign = m_sema.getTypeContext().getFuncSignature(func_tid);

        auto func = m_arena.alloc<Function>();

        if (attr & STATIC)
            func->is_static = true;

        current_func  = func;
        func->name    = ident.getContent();
        func->type_id = func_tid;

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
    Node *varDecl(uint32_t attr)
    {
        if (attr & TYPEDEF)
        {
            typedefDecl();
            return nullptr;
        }

        TypeId basety = declSpec();

        auto node     = m_arena.alloc<BlockNode>(tok.getToken());
        int var_count = 0;

        while (!tok.tryConsumeToken(";"))
        {
            if (var_count++ > 0)
                tok.consumeToken(",");

            Token ident;

            TypeId final_tid = declarator(basety, ident);
            auto type        = m_sema.getTypeContext().getType(final_tid);
            if (type.kind == TypeKind::VOID)
                DiagnosticEngine::errorOnTok(ident, "variable declared void");

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

        node->tok = tok.getPrev();
        return node;
    }

    void typedefDecl()
    {
        auto basety = declSpec();

        int count = 0;

        while (!tok.tryConsumeToken(";"))
        {
            if (count++ > 0)
                tok.consumeToken(",");

            Token ident;
            TypeId final_tid = declarator(basety, ident);

            auto typedef_sym     = m_arena.alloc<Typedef>();
            typedef_sym->name    = ident.getContent();
            typedef_sym->type_id = final_tid;
            m_sym_table.insertIdent(ident.getContent(), typedef_sym);
        }
    }

private:
    Node *stmt()
    {
        if (isTypename())
        {
            return varDecl<IsLocal>(0);
        }
        else if (isAttr())
        {
            auto &token = tok.getToken();
            if (tok.tryConsumeToken("typedef"))
                typedefDecl();
            return m_arena.alloc<BlockNode>(token);
        }

        if (tok.tryConsumeToken("return"))
        {
            auto kw_tok = tok.getPrev();
            auto node   = expr();

            auto &func_sign = m_sema.getTypeContext().getFuncSignature(current_func->type_id);
            if (node->type_id != func_sign.return_type_id)
                node = m_arena.alloc<TypeCastNode>(node, func_sign.return_type_id, kw_tok);
            node = m_arena.alloc<ReturnNode>(node, kw_tok);
            tok.consumeToken(";");
            return node;
        }

        if (tok.tryConsumeToken("if"))
        {
            auto kw_tok = tok.getPrev();
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
            auto kw_tok = tok.getPrev();
            auto node   = m_arena.alloc<ForNode>(kw_tok);
            node->tok   = kw_tok;
            m_sym_table.enterScope();
            tok.consumeToken("(");
            if (isTypename())
                node->init = varDecl<IsLocal>(0);
            else
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
            m_sym_table.leaveScope();
            return node;
        }

        if (tok.tryConsumeToken("while"))
        {
            auto kw_tok = tok.getPrev();
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
            node->stmts.push_back(stmt());

        m_sym_table.leaveScope();
        return node;
    }

    Node *exprStmt()
    {
        if (tok.tryConsumeToken(";"))
            return m_arena.alloc<BlockNode>(tok.getPrev());

        auto node = m_arena.alloc<ExprStmtNode>(expr(), tok.getToken());
        tok.consumeToken(";");
        return node;
    }

    Node *expr()
    {
        auto node = assign();

        while (tok.tryConsumeToken(","))
        {
            auto &op_tok  = tok.getPrev();
            auto rhs      = assign();
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
            auto &op_tok         = tok.getPrev();
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
                auto &op_tok  = tok.getPrev();
                auto rhs      = relational();
                node          = m_arena.alloc<BinaryNode>(NodeKind::EQ, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("!="))
            {
                auto &op_tok  = tok.getPrev();
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
                auto &op_tok  = tok.getPrev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::LE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken("<"))
            {
                auto &op_tok  = tok.getPrev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::LT, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">="))
            {
                auto &op_tok  = tok.getPrev();
                auto rhs      = add();
                node          = m_arena.alloc<BinaryNode>(NodeKind::GE, node, rhs, op_tok);
                node->type_id = m_sema.getTypeContext().getIntTypeId();
                continue;
            }

            if (tok.tryConsumeToken(">"))
            {
                auto &op_tok  = tok.getPrev();
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
                auto &op_tok = tok.getPrev();
                node         = m_sema.buildAdd(node, mul(), op_tok);
                continue;
            }

            if (tok.tryConsumeToken("-"))
            {
                auto &op_tok = tok.getPrev();
                node         = m_sema.buildSub(node, mul(), op_tok);
                continue;
            }

            return node;
        }
    }

    Node *mul()
    {
        auto node = cast();

        while (true)
        {
            if (tok.tryConsumeToken("*"))
            {
                auto &op_tok = tok.getPrev();
                node         = m_sema.buildMul(node, cast(), op_tok);
                continue;
            }

            if (tok.tryConsumeToken("/"))
            {
                auto &op_tok = tok.getPrev();
                node         = m_sema.buildDiv(node, cast(), op_tok);
                continue;
            }

            if (tok.tryConsumeToken("%"))
            {
                auto &op_tok = tok.getPrev();
                node         = m_sema.buildMod(node, cast(), op_tok);
                continue;
            }

            return node;
        }
    }

    Node *cast()
    {
        if (tok.isToken("(") && isTypename(tok.lookAhead(1)))
        {
            auto &op_tok = tok.getToken();
            tok.consumeToken("(");
            auto basety    = declSpec();
            auto final_tid = abstractDeclarator(basety);
            tok.consumeToken(")");
            return m_arena.alloc<TypeCastNode>(cast(), final_tid, op_tok);
        }

        return unary();
    }

    Node *unary()
    {
        if (tok.tryConsumeToken("+"))
            return cast();

        if (tok.tryConsumeToken("-"))
        {
            auto &op_tok  = tok.getPrev();
            auto node     = m_arena.alloc<UnaryNode>(NodeKind::NEG, cast(), op_tok);
            node->type_id = node->lhs->type_id;
            return node;
        }

        if (tok.tryConsumeToken("*"))
        {
            auto &op_tok = tok.getPrev();
            return m_sema.buildDeref(cast(), op_tok);
        }

        if (tok.tryConsumeToken("&"))
        {
            auto &op_tok = tok.getPrev();
            return m_sema.buildAddr(cast(), op_tok);
        }

        if (tok.tryConsumeToken("sizeof"))
        {
            auto &op_tok = tok.getPrev();
            TypeId final_tid;

            if (tok.isToken("(") && isTypename(tok.lookAhead(1)))
            {
                tok.consumeToken("(");
                auto basety = declSpec();
                final_tid   = abstractDeclarator(basety);
                tok.consumeToken(")");
            }
            else
            {
                auto node = cast();
                final_tid = node->type_id;
            }
            auto size         = m_sema.getTypeSize(final_tid);
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
                auto &op_tok = tok.getPrev();
                auto idx     = expr();
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
                node = m_sema.buildDeref(node, tok.getPrev());
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

            auto sym = m_sym_table.lookupIdent(token.getContent());
            if (!sym)
                DiagnosticEngine::errorOnTok(token, "undeclared identifier '{}'", token.getContent());

            if (tok.tryConsumeToken("("))
            {
                if (sym->sym_type != SymbolType::Function)
                    DiagnosticEngine::errorOnTok(token, "identifier '{}' is not a function", token.getContent());

                return funCall(token);
            }

            if (auto var = dynamic_cast<Variable *>(sym))
            {
                auto node     = m_arena.alloc<VarNode>(var, token);
                node->type_id = var->type_id;
                return node;
            }

            if (auto e = dynamic_cast<Enum *>(sym))
            {
                auto node     = m_arena.alloc<NumNode>(e->val);
                node->type_id = e->type_id;
                return node;
            }

            DiagnosticEngine::errorOnTok(token, "identifier '{}' is not a variable or a enum member", token.getContent());
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
            auto node = m_arena.alloc<NumNode>(token.val, tok.getToken());
            tok.skipToken();
            node->type_id = m_sema.getTypeContext().getIntTypeId();
            return node;
        }

        DiagnosticEngine::errorOnTok(token, "expect a num but got '{}'", token.getContent());
    }

private:
    TypeId declSpec()
    {
        enum TypeBits
        {
            VOID  = 1 << 0,
            BOOL  = 1 << 2,
            CHAR  = 1 << 4,
            SHORT = 1 << 6,
            INT   = 1 << 8,
            LONG  = 1 << 10,
            OTHER = 1 << 12,
        };

        TypeId ty   = m_sema.getTypeContext().getIntTypeId();
        int counter = 0;

        while (isTypename())
        {
            auto &token = tok.getToken();

            if (token.getContent() == "struct" || token.getContent() == "union" || token.getContent() == "enum")
            {
                if (counter > 0)
                    DiagnosticEngine::errorOnTok(tok.getToken(), "invalid type combination");

                if (tok.tryConsumeToken("struct"))
                    ty = structAndUnionDecl<IsStruct>();
                else if (tok.tryConsumeToken("union"))
                    ty = structAndUnionDecl<IsUnion>();
                else if (tok.tryConsumeToken("enum"))
                    ty = enumDecl();

                counter += OTHER;
                continue;
            }

            if (token.kind == TokenKind::IDENT)
            {
                auto symbol = m_sym_table.lookupIdent(token.getContent());
                if (symbol && symbol->sym_type == SymbolType::Typedef)
                {
                    if (counter > 0)
                        break;

                    ty = symbol->type_id;
                    counter += OTHER;
                    tok.skipToken();
                    break;
                }
            }

            if (tok.tryConsumeToken("void"))
                counter += VOID;
            else if (tok.tryConsumeToken("_Bool") || tok.tryConsumeToken("bool"))
                counter += BOOL;
            else if (tok.tryConsumeToken("char"))
                counter += CHAR;
            else if (tok.tryConsumeToken("short"))
                counter += SHORT;
            else if (tok.tryConsumeToken("int"))
                counter += INT;
            else if (tok.tryConsumeToken("long"))
                counter += LONG;
            else
                break;
        }

        if (counter == 0)
            DiagnosticEngine::errorOnTok(tok.getToken(), "typename expected");

        switch (counter)
        {
        case VOID:
            return m_sema.getTypeContext().getVoidTypeId();
        case BOOL:
            return m_sema.getTypeContext().getBoolTypeId();
        case CHAR:
            return m_sema.getTypeContext().getCharTypeId();
        case SHORT:
        case SHORT + INT:
            return m_sema.getTypeContext().getShortTypeId();
        case INT:
            return m_sema.getTypeContext().getIntTypeId();
        case LONG:
        case LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
            return m_sema.getTypeContext().getLongTypeId();
        case OTHER:
            return ty;
        default:
            DiagnosticEngine::errorOnTok(tok.getToken(), "invalid type combination");
        }
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

    TypeId abstractDeclarator(TypeId base_tid)
    {
        TypeId current_tid = base_tid;

        while (tok.tryConsumeToken("*"))
            current_tid = m_sema.getTypeContext().getPointerTypeId(current_tid);

        if (tok.tryConsumeToken("("))
        {
            auto begin_tok = tok;

            abstractDeclarator(base_tid);
            tok.consumeToken(")");

            current_tid = declSuffix(current_tid);

            RAIITokReverter rvt(tok);
            tok = begin_tok;
            return abstractDeclarator(current_tid);
        }

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
        auto node = m_arena.alloc<FuncCallNode>(ident.getContent(), ident);

        auto func       = static_cast<Function *>(m_sym_table.lookupIdent(ident.getContent()));
        auto &func_sign = m_sema.getTypeContext().getFuncSignature(func->type_id);

        node->type_id = func_sign.return_type_id;

        bool is_first_arg{true};
        while (!tok.tryConsumeToken(")"))
        {
            if (!is_first_arg)
                tok.consumeToken(",");

            node->args.emplace_back(assign());
            is_first_arg = false;
        }

        for (size_t i = 0; i < func_sign.param_types.size(); ++i)
        {
            if (node->args[i]->type_id != func_sign.param_types[i])
                node->args[i] = m_arena.alloc<TypeCastNode>(node->args[i], func_sign.param_types[i]);
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

    std::int64_t getNumber()
    {
        std::int64_t value;
        auto content = tok.getToken().getContent();
        std::from_chars(content.begin(), content.end(), value);
        return value;
    }

    bool isTypename(const Token &token)
    {
        std::array<std::string_view, 10> typenames{"void"sv, "_Bool"sv, "bool"sv, "char"sv, "short"sv, "int"sv, "long"sv, "struct"sv, "union"sv, "enum"sv};
        auto it = std::find(typenames.begin(), typenames.end(), token.getContent());
        if (it != typenames.end())
            return true;

        if (token.kind != TokenKind::IDENT)
            return false;

        auto symbol = m_sym_table.lookupIdent(token.getContent());
        return symbol && symbol->sym_type == SymbolType::Typedef;
    }

    bool isTypename()
    {
        return isTypename(tok.getToken());
    }

    bool isAttr()
    {
        return tok.isToken("typedef") || tok.isToken("static");
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
        static int id        = 0;
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
    Function *current_func;
};