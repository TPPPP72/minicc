#pragma once

#include <Scope/Symbol.hpp>

struct Variable : Symbol
{
    int offset;
    int init_val{};
    bool has_init{};
};