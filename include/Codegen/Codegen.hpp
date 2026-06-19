#pragma once

#include <Scope/Variable.hpp>
#include <Scope/Function.hpp>
#include <Codegen/Assembler.hpp>
#include <Diag/Diag.hpp>
#include <Sema/Sema.hpp>

using namespace std::string_view_literals;

class Codegen
{
public:
    Codegen(Sema &sema) : m_sema(sema) {}

    void genProg(std::vector<Symbol *> &symbols, std::string_view path)
    {
        Assembler::initialize(path);
        assign_lvar_offsets(symbols);
        emitData(symbols);
        emitText(symbols);
        Assembler::close();
    }

private:
    void genStmt(Node *node, std::string_view func_name)
    {
        switch (node->kind)
        {
        case NodeKind::EXPR_STMT:
        {
            auto expr_stmt_node = static_cast<ExprStmtNode *>(node);
            genExpr(expr_stmt_node->expr);
            return;
        }
        case NodeKind::RETURN:
        {
            auto ret_node = static_cast<ReturnNode *>(node);
            genExpr(ret_node->expr);
            Assembler::jmp(std::format(".L.return.{}", func_name));
            return;
        }
        case NodeKind::BLOCK:
        {
            auto block_node = static_cast<BlockNode *>(node);
            for (auto stmt : block_node->stmts)
                genStmt(stmt, func_name);
            return;
        }
        case NodeKind::IF:
        {
            auto if_node = static_cast<IfNode *>(node);
            int temp     = ++count;
            genExpr(if_node->cond);
            Assembler::cmp(Imm{0}, Reg{"rax"});
            Assembler::je(std::format(".L.else.{}", temp));
            genStmt(if_node->then, func_name);
            Assembler::jmp(std::format(".L.end.{}", temp));
            Assembler::label(std::format(".L.else.{}", temp));
            if (if_node->els)
                genStmt(if_node->els, func_name);
            Assembler::label(std::format(".L.end.{}", temp));
            return;
        }
        case NodeKind::FOR:
        {
            auto for_node = static_cast<ForNode *>(node);
            int temp      = ++count;
            if (for_node->init)
                genStmt(for_node->init, func_name);
            Assembler::label(std::format(".L.begin.{}", temp));
            if (for_node->cond)
            {
                genExpr(for_node->cond);
                Assembler::cmp(Imm{0}, Reg{"rax"});
                Assembler::je(std::format(".L.end.{}", temp));
            }
            genStmt(for_node->then, func_name);
            if (for_node->inc)
                genExpr(for_node->inc);
            Assembler::jmp(std::format(".L.begin.{}", temp));
            Assembler::label(std::format(".L.end.{}", temp));
            return;
        }
        default:
            break;
        }

        DiagnosticEngine::errorOnTok(node->tok, "error statement");
    }

    void genExpr(Node *node)
    {
        switch (node->kind)
        {
        case NodeKind::NUM:
        {
            auto num_node = static_cast<NumNode *>(node);
            Assembler::mov(Imm{num_node->val}, Reg{"rax"});
            return;
        }
        case NodeKind::NEG:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genExpr(unary_node->lhs);
            Assembler::neg(Reg{"rax"});
            return;
        }
        case NodeKind::VAR:
        {
            genAddr(node);
            load(node);
            return;
        }
        case NodeKind::DEREF:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genExpr(unary_node->lhs);
            load(node);
            return;
        }
        case NodeKind::ADDR:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genAddr(unary_node->lhs);
            return;
        }
        case NodeKind::ASSIGN:
        {
            auto binary_node = static_cast<BinaryNode *>(node);
            genAddr(binary_node->lhs);
            Assembler::push(Reg{"rax"});
            genExpr(binary_node->rhs);
            store(node);
            return;
        }
        case NodeKind::FUNCALL:
        {
            auto func_node = static_cast<FuncCallNode *>(node);
            for (auto arg : func_node->args)
            {
                genExpr(arg);
                Assembler::push(Reg{"rax"});
            }

            for (auto i = func_node->args.size(); i > 0; --i)
                Assembler::pop(Reg{argreg64[i - 1]});

            Assembler::mov(Imm{0}, Reg{"rax"});
            Assembler::call(func_node->func_name);
            return;
        }
        default:
            break;
        }

        auto binary_node = static_cast<BinaryNode *>(node);
        genExpr(binary_node->rhs);
        Assembler::push(Reg{"rax"});
        genExpr(binary_node->lhs);
        Assembler::pop(Reg{"rdi"});

        switch (binary_node->kind)
        {
        case NodeKind::ADD:
            Assembler::add(Reg{"rdi"}, Reg{"rax"});
            return;
        case NodeKind::SUB:
            Assembler::sub(Reg{"rdi"}, Reg{"rax"});
            return;
        case NodeKind::MUL:
            Assembler::imul(Reg{"rdi"}, Reg{"rax"});
            return;
        case NodeKind::DIV:
            Assembler::idiv(Reg{"rdi"});
            return;
        case NodeKind::MOD:
            Assembler::idiv(Reg{"rdi"});
            Assembler::mov(Reg{"rdx"}, Reg{"rax"});
            return;
        case NodeKind::EQ:
        case NodeKind::NE:
        case NodeKind::LE:
        case NodeKind::LT:
        case NodeKind::GE:
        case NodeKind::GT:
            Assembler::cmp(Reg{"rdi"}, Reg{"rax"});
            switch (binary_node->kind)
            {
            case NodeKind::EQ:
                Assembler::sete(Reg{"al"});
                break;
            case NodeKind::NE:
                Assembler::setne(Reg{"al"});
                break;
            case NodeKind::LE:
                Assembler::setle(Reg{"al"});
                break;
            case NodeKind::LT:
                Assembler::setl(Reg{"al"});
                break;
            case NodeKind::GE:
                Assembler::setge(Reg{"al"});
                break;
            case NodeKind::GT:
                Assembler::setg(Reg{"al"});
                break;
            default:
                break;
            }
            Assembler::movzb(Reg{"al"}, Reg{"rax"});
            return;
        default:
            break;
        }

        DiagnosticEngine::errorOnTok(node->tok, "invalid expression");
    }

private:
    void genAddr(Node *node)
    {
        switch (node->kind)
        {
        case NodeKind::VAR:
        {
            auto var_node = static_cast<VarNode *>(node);
            if (var_node->var->is_local)
                Assembler::lea(Mem{"rbp", var_node->var->offset}, Reg{"rax"});
            else
                Assembler::lea(Mem{"rip", var_node->var->name}, Reg{"rax"});
            return;
        }
        case NodeKind::DEREF:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genExpr(unary_node->lhs);
            return;
        }
        default:
            break;
        }

        DiagnosticEngine::errorOnTok(node->tok, "not a lvalue");
    }

private:
    void load(Node *node)
    {
        auto type = m_sema.getTypeContext().getType(node->type_id);
        if (type.kind == TypeKind::ARRAY)
            return;

        if (type.size == 1)
            Assembler::movsbq(Mem{"rax"}, Reg{"rax"});
        else
            Assembler::mov(Mem{"rax"}, Reg{"rax"});
    }

    void store(Node *node)
    {
        auto type = m_sema.getTypeContext().getType(node->type_id);
        Assembler::pop(Reg{"rdi"});

        if (type.size == 1)
            Assembler::mov(Reg{"al"}, Mem{"rdi"});
        else
            Assembler::mov(Reg{"rax"}, Mem{"rdi"});
    }

    void emitData(const std::vector<Symbol *> &symbols){
        for(auto sym : symbols){
            if (auto var = dynamic_cast<Variable *>(sym))
            {
                Assembler::directive("data");
                Assembler::globl(var->name);
                Assembler::label(var->name);
                if (var->is_string_literal)
                {
                    Assembler::string_lit(escapeToAssembly(var->string_data));
                }
                else if (var->has_int_init)
                {
                    auto size = m_sema.getTypeSize(var->type_id);
                    if (size == 8)
                        Assembler::quad(var->int_init_val);
                    else
                        Assembler::long_val(var->int_init_val);
                }
                else
                {
                    Assembler::zero(m_sema.getTypeSize(var->type_id));
                }
            }
        }
    }

    void emitText(const std::vector<Symbol *> &symbols)
    {
        for (auto sym : symbols)
        {
            if (auto func = dynamic_cast<Function *>(sym))
            {
                Assembler::globl(func->name);
                Assembler::directive("text");
                Assembler::label(func->name);

                Assembler::push(Reg{"rbp"});
                Assembler::mov(Reg{"rsp"}, Reg{"rbp"});
                Assembler::sub(Imm{func->stack_size}, Reg{"rsp"});

                int i = 0;
                for (auto var : func->params)
                {
                    if (m_sema.getTypeSize(var->type_id) == 1)
                        Assembler::mov(Reg{argreg8[i++],}, Mem{"rbp", static_cast<Variable *>(var)->offset});
                    else
                        Assembler::mov(Reg{argreg64[i++]}, Mem{"rbp", static_cast<Variable *>(var)->offset});
                }

                auto block_node = static_cast<BlockNode *>(func->body);
                for (auto stmt : block_node->stmts)
                    genStmt(stmt, func->name);

                Assembler::label(std::format(".L.return.{}", func->name));
                Assembler::mov(Reg{"rbp"}, Reg{"rsp"});
                Assembler::pop(Reg{"rbp"});
                Assembler::ret();
            }
        }
    }

    int align_to(int n, int align)
    {
        return (n + align - 1) / align * align;
    }

    void assign_lvar_offsets(const std::vector<Symbol *> &symbols)
    {
        for (auto sym : symbols)
        {
            if (auto func = dynamic_cast<Function *>(sym))
            {
                int offset = 0;
                for (auto it = func->locals.rbegin(); it != func->locals.rend(); ++it)
                {
                    auto var = static_cast<Variable *>(*it);
                    offset += m_sema.getTypeSize((*it)->type_id);
                    var->offset = -offset;
                }
                func->stack_size = align_to(offset, 16);
            }
        }
    }

    std::string escapeToAssembly(const std::string &src)
    {
        std::string dst;
        for (char c : src)
        {
            switch (c)
            {
            case '\n':
                dst += "\\n";
                break;
            case '\t':
                dst += "\\t";
                break;
            case '\r':
                dst += "\\r";
                break;
            case '\b':
                dst += "\\b";
                break;
            case '"':
                dst += "\\\"";
                break;
            case '\\':
                dst += "\\\\";
                break;

            case '\a':
                dst += "\\007";
                break;
            case '\v':
                dst += "\\013";
                break;
            case '\f':
                dst += "\\014";
                break;
            case '\e':
                dst += "\\033";
                break;

            default:
                if (c < 32)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\%03o", static_cast<unsigned char>(c));
                    dst += buf;
                }
                else
                {
                    dst += c;
                }
                break;
            }
        }
        return dst;
    }

private:
    std::array<std::string_view, 6> argreg8{"dil"sv, "sil"sv, "dl"sv, "cl"sv, "r8b"sv, "r9b"sv};
    std::array<std::string_view, 6> argreg64{"rdi"sv, "rsi"sv, "rdx"sv, "rcx"sv, "r8"sv, "r9"sv};
    Sema &m_sema;
    int count{};
};