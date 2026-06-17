#pragma once

#include <AST/Node.hpp>
#include <Scope/Symbol.hpp>

struct Function : Symbol {
    std::vector<Symbol*> params;
    std::vector<Symbol*> locals;
    Node* body = nullptr;
    int stack_size{};
};