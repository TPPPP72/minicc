#pragma once

#include <AST/Node.hpp>

struct Function
{
    Node *body;
    Object *locals;
    std::int32_t stack_size;
};