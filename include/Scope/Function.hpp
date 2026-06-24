#pragma once

#include <AST/Node.hpp>
#include <Scope/Symbol.hpp>

struct Function : Symbol
{
    std::vector<Symbol *> params;
    std::vector<Variable *> locals;
    Node *body = nullptr;
    int stack_size{};
    bool is_definition{};
};