#pragma once

#include <AST/Function.hpp>
#include <Codegen/Assembler.hpp>
#include <Diag/Diag.hpp>

class Codegen
{
public:
    void generate(Function *func)
    {
        assign_lvar_offsets(func);

        std::cout << "  .globl main\n";
        Assembler::label("main");

        Assembler::push("rbp");
        Assembler::mov("rsp", "rbp");
        Assembler::sub(func->stack_size, "rsp");

        auto block_node = static_cast<BlockNode *>(func->body);
        for (auto stmt : block_node->stmts)
            genStmt(stmt);

        Assembler::label(".L.return");
        Assembler::mov("rbp", "rsp");
        Assembler::pop("rbp");
        Assembler::ret();
    }

private:
    void genStmt(Node *node)
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
            std::cout << "  jmp .L.return\n";
            return;
        }
        case NodeKind::BLOCK:
        {
            auto block_node = static_cast<BlockNode *>(node);
            for (auto stmt : block_node->stmts)
                genStmt(stmt);
            return;
        }
        case NodeKind::IF:
        {
            auto if_node = static_cast<IfNode *>(node);
            int temp     = ++count;
            genExpr(if_node->cond);
            Assembler::cmp(0, "rax");
            Assembler::je(std::format(".L.else.{}", temp));
            genStmt(if_node->then);
            Assembler::jmp(std::format(".L.end.{}", temp));
            Assembler::label(std::format(".L.else.{}", temp));
            if (if_node->els)
                genStmt(if_node->els);
            Assembler::label(std::format(".L.end.{}", temp));
            return;
        }
        case NodeKind::FOR:
        {
            auto for_node = static_cast<ForNode *>(node);
            int temp      = ++count;
            if (for_node->init)
                genStmt(for_node->init);
            Assembler::label(std::format(".L.begin.{}", temp));
            if (for_node->cond)
            {
                genExpr(for_node->cond);
                Assembler::cmp(0, "rax");
                Assembler::je(std::format(".L.end.{}", temp));
            }
            genStmt(for_node->then);
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
            std::cout << "  mov (%rax), %rax\n";
            return;
        }
        case NodeKind::DEREF:
        {
            auto unary_node = static_cast<UnaryNode *>(node);
            genExpr(unary_node->lhs);
            std::cout << "  mov (%rax), %rax\n";
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
            Assembler::pop("rdi");
            std::cout << "  mov %rax, (%rdi)\n";
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
        case NodeKind::VAR:{
            auto var_node = static_cast<VarNode *>(node);
            Assembler::lea(var_node->var->offset, "rbp", "rax");
            return;
        }
        case NodeKind::DEREF:{
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
    int align_to(int n, int align)
    {
        return (n + align - 1) / align * align;
    }

    void assign_lvar_offsets(Function *func)
    {
        int offset = 0;
        for (auto var = func->locals; var; var = var->next)
        {
            offset += 8;
            var->offset = -offset;
        }
        func->stack_size = align_to(offset, 16);
    }

private:
    int count{};
};