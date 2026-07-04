#pragma once

#include <AST/Type.hpp>
#include <string_view>

enum class SymbolType
{
    Variable,
    Function,
    Typedef
};

struct Symbol
{
    std::string_view name;
    SymbolType sym_type;
    TypeId type_id;

    Symbol(SymbolType st) : sym_type(st) {}
    virtual ~Symbol() = default;
};