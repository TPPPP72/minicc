#pragma once

#include <AST/Node.hpp>
#include <vector>

struct Function
{
    std::vector<Object *> params;
    std::vector<Object *> locals;
    std::string_view name;
    Node *body;
    std::int32_t stack_size;
};