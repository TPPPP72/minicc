#pragma once

#include <Assembler.hpp>
#include <Diag.hpp>
#include <minicc.hpp>

class Codegen
{
public:
    void generate(Node *node)
    {
        std::cout << "  .globl main\n";
        Assembler::label("main");

        for (Node *n = node; n; n = n->next)
            gen_stmt(n);

        Assembler::ret();
    }

private:
    void gen_stmt(Node *node)
    {
        if (node->type == NodeType::EXPR_STMT)
        {
            gen_expr(node->lhs);
            return;
        }

        DiagnosticEngine::error("error statement");
    }

    void gen_expr(Node *node)
    {
        switch (node->type)
        {
        case NodeType::NUM:
            Assembler::mov(node->val, "rax");
            return;
        case NodeType::NEG:
            Assembler::neg("rax");
            return;
        default:
            __builtin_unreachable();
        }

        gen_expr(node->rhs);
        Assembler::push();
        gen_expr(node->lhs);
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
        default:
            __builtin_unreachable();
        }

        DiagnosticEngine::error("invalid expression");
    }
};