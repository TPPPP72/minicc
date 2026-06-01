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

        for (auto n = func->body; n; n = n->next)
            gen_stmt(n);

        std::cout << ".L.return:\n";
        Assembler::mov("rbp", "rsp");
        Assembler::pop("rbp");
        Assembler::ret();
    }

private:
    void gen_stmt(Node *node)
    {
        switch (node->type)
        {
        case NodeType::EXPR_STMT:
            genExpr(node->lhs);
            return;
        case NodeType::RETURN:
            genExpr(node->lhs);
            std::cout << "  jmp .L.return\n";
            return;
        case NodeType::BLOCK:
            for (Node *n = node->body; n; n = n->next)
                gen_stmt(n);
            return;
        default:
            break;
        }

        DiagnosticEngine::error("error statement");
    }

    void genExpr(Node *node)
    {
        switch (node->type)
        {
        case NodeType::NUM:
            Assembler::mov(node->val, "rax");
            return;
        case NodeType::NEG:
            Assembler::neg("rax");
            return;
        case NodeType::VAR:
            genAddr(node);
            std::cout << "  mov (%rax), %rax\n";
            return;
        case NodeType::ASSIGN:
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

        switch (node->type)
        {
        case NodeType::ADD:
            Assembler::add("rdi", "rax");
            return;
        case NodeType::SUB:
            Assembler::sub("rdi", "rax");
            return;
        case NodeType::MUL:
            Assembler::imul("rdi", "rax");
            return;
        case NodeType::DIV:
            Assembler::idiv("rdi", "rax");
            return;
        case NodeType::MOD:
            Assembler::idiv("rdi", "rax");
            Assembler::mov("rdx", "rax");
            return;
        case NodeType::EQ:
        case NodeType::NE:
        case NodeType::LE:
        case NodeType::LT:
        case NodeType::GE:
        case NodeType::GT:
            Assembler::cmp("rdi", "rax");
            switch (node->type)
            {
            case NodeType::EQ:
                Assembler::sete("al");
                break;
            case NodeType::NE:
                Assembler::setne("al");
                break;
            case NodeType::LE:
                Assembler::setle("al");
                break;
            case NodeType::LT:
                Assembler::setl("al");
                break;
            case NodeType::GE:
                Assembler::setge("al");
                break;
            case NodeType::GT:
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

        DiagnosticEngine::error("invalid expression");
    }

private:
    void genAddr(Node *node)
    {
        if (node->type == NodeType::VAR)
        {
            Assembler::lea(node->var->offset, "rbp", "rax");
            return;
        }

        DiagnosticEngine::error("not a lvalue");
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
};