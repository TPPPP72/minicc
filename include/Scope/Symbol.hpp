#pragma once

#include <AST/Type.hpp>
#include <string_view>

struct Symbol
{
    std::string_view name;
    TypeId type_id;
    bool is_local{};
    virtual ~Symbol() = default;
};