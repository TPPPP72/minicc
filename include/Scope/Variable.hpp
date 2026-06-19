#pragma once

#include <Scope/Symbol.hpp>
#include <string>

struct Variable : Symbol
{
    std::string string_data;
    int offset{};
    int int_init_val{};
    bool is_string_literal{};
    bool has_int_init{};
};