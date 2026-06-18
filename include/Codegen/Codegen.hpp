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
    void genProg(std::vector<Symbol *> &symbols)
    {
        assign_lvar_offsets(symbols);
        emitData(symbols);
        emitText(symbols);
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
            Assembler::cmp(0, "rax");
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
                Assembler::cmp(0, "rax");
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
            Assembler::mov(num_node->val, "rax");
            return;
        }
        case NodeKind::NEG:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genExpr(unary_node->lhs);
            Assembler::neg("rax");
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
            Assembler::push("rax");
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
                Assembler::push("rax");
            }

            for (auto i = func_node->args.size(); i > 0; --i)
                Assembler::pop(argreg64[i - 1]);

            Assembler::mov(0, "rax");
            Assembler::call(func_node->func_name);
            return;
        }
        default:
            break;
        }

        auto binary_node = static_cast<BinaryNode *>(node);
        genExpr(binary_node->rhs);
        Assembler::push("rax");
        genExpr(binary_node->lhs);
        Assembler::pop("rdi");

        switch (binary_node->kind)
        {
        case NodeKind::ADD:
            Assembler::add("rdi", "rax");
            return;
        case NodeKind::SUB:
            Assembler::sub("rdi", "rax");
            return;
        case NodeKind::MUL:
            Assembler::imul("rdi", "rax");
            return;
        case NodeKind::DIV:
            Assembler::idiv("rdi", "rax");
            return;
        case NodeKind::MOD:
            Assembler::idiv("rdi", "rax");
            Assembler::mov("rdx", "rax");
            return;
        case NodeKind::EQ:
        case NodeKind::NE:
        case NodeKind::LE:
        case NodeKind::LT:
        case NodeKind::GE:
        case NodeKind::GT:
            Assembler::cmp("rdi", "rax");
            switch (binary_node->kind)
            {
            case NodeKind::EQ:
                Assembler::sete("al");
                break;
            case NodeKind::NE:
                Assembler::setne("al");
                break;
            case NodeKind::LE:
                Assembler::setle("al");
                break;
            case NodeKind::LT:
                Assembler::setl("al");
                break;
            case NodeKind::GE:
                Assembler::setge("al");
                break;
            case NodeKind::GT:
                Assembler::setg("al");
                break;
            default:
                break;
            }
            Assembler::movzb("al", "rax");
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
                Assembler::lea(var_node->var->offset, "rbp", "rax");
            else
                Assembler::lea(var_node->var->name, "rip", "rax");
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
            std::cout << "  movsbq (%rax), %rax\n";
        else
            std::cout << "  mov (%rax), %rax\n";
    }

    void store(Node *node)
    {
        auto type = m_sema.getTypeContext().getType(node->type_id);
        Assembler::pop("rdi");

        if (type.size == 1)
            std::cout << "  mov %al, (%rdi)\n";
        else
            std::cout << "  mov %rax, (%rdi)\n";
    }

    void emitData(const std::vector<Symbol *> &symbols){
        for(auto sym : symbols){
            if (auto var = dynamic_cast<Variable *>(sym))
            {
                std::cout << "  .data\n";
                std::cout << "  .globl " << var->name << '\n';
                std::cout << var->name << ":\n";
                if (!var->has_init)
                    std::cout << "  .zero  " << m_sema.getTypeSize(var->type_id) << '\n';
                else
                    std::cout << "  .quad " << var->init_val << '\n';
            }
        }
    }

    void emitText(const std::vector<Symbol *> &symbols)
    {
        for (auto sym : symbols)
        {
            if (auto func = dynamic_cast<Function *>(sym))
            {
                std::cout << "  .globl " << func->name << '\n';
                std::cout << "  .text\n";
                Assembler::label(func->name);

                Assembler::push("rbp");
                Assembler::mov("rsp", "rbp");
                Assembler::sub(func->stack_size, "rsp");

                int i = 0;
                for (auto var : func->params)
                {
                    if (m_sema.getTypeSize(var->type_id) == 1)
                        std::cout << "  mov %" << argreg8[i++] << ", " << static_cast<Variable *>(var)->offset << "(%rbp)\n";
                    else
                        std::cout << "  mov %" << argreg64[i++] << ", " << static_cast<Variable *>(var)->offset << "(%rbp)\n";
                }

                auto block_node = static_cast<BlockNode *>(func->body);
                for (auto stmt : block_node->stmts)
                    genStmt(stmt, func->name);

                Assembler::label(std::format(".L.return.{}", func->name));
                Assembler::mov("rbp", "rsp");
                Assembler::pop("rbp");
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

private:
    std::array<std::string_view, 6> argreg8{"dil"sv, "sil"sv, "dl"sv, "cl"sv, "r8b"sv, "r9b"sv};
    std::array<std::string_view, 6> argreg64{"rdi"sv, "rsi"sv, "rdx"sv, "rcx"sv, "r8"sv, "r9"sv};
    Sema &m_sema;
    int count{};
};