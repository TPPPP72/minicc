#pragma once

#include "AST/Node.hpp"
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

        for (auto n = func->body; n; n = n->next)
            genStmt(n);

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
            genExpr(node->lhs);
            return;
        }
        case NodeKind::RETURN:
        {
            genExpr(node->lhs);
            std::cout << "  jmp .L.return\n";
            return;
        }
        case NodeKind::BLOCK:
        {
            for (Node *n = node->body; n; n = n->next)
                genStmt(n);
            return;
        }
        case NodeKind::IF:
        {
            int temp = ++count;
            genExpr(node->cond);
            Assembler::cmp(0, "rax");
            Assembler::je(std::format(".L.else.{}", temp));
            genStmt(node->then);
            Assembler::jmp(std::format(".L.end.{}", temp));
            Assembler::label(std::format(".L.else.{}", temp));
            if (node->els)
                genStmt(node->els);
            Assembler::label(std::format(".L.end.{}", temp));
            return;
        }
        case NodeKind::FOR:
        {
            int temp = ++count;
            if (node->init)
                genStmt(node->init);
            Assembler::label(std::format(".L.begin.{}", temp));
            if (node->cond)
            {
                genExpr(node->cond);
                Assembler::cmp(0, "rax");
                Assembler::je(std::format(".L.end.{}", temp));
            }
            genStmt(node->then);
            if (node->inc)
                genExpr(node->inc);
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
            Assembler::mov(node->val, "rax");
            return;
        case NodeKind::NEG:
            genExpr(node->lhs);
            Assembler::neg("rax");
            return;
        case NodeKind::VAR:
            genAddr(node);
            std::cout << "  mov (%rax), %rax\n";
            return;
        case NodeKind::DEREF:
            genExpr(node->lhs);
            std::cout << "  mov (%rax), %rax\n";
            return;
        case NodeKind::ADDR:
            genAddr(node->lhs);
            return;
        case NodeKind::ASSIGN:
            genAddr(node->lhs);
            Assembler::push("rax");
            genExpr(node->rhs);
            Assembler::pop("rdi");
            std::cout << "  mov %rax, (%rdi)\n";
            return;
        default:
            break;
        }

        genExpr(node->rhs);
        Assembler::push("rax");
        genExpr(node->lhs);
        Assembler::pop("rdi");

        switch (node->kind)
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
            switch (node->kind)
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
        if (node->kind == NodeKind::VAR)
        {
        }
        switch (node->kind)
        {
        case NodeKind::VAR:
            Assembler::lea(node->var->offset, "rbp", "rax");
            return;
        case NodeKind::DEREF:
            genExpr(node->lhs);
            return;
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