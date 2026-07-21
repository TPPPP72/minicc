#pragma once

#include <Scope/Symbol.hpp>

struct Enum : Symbol
{
    int64_t val;

    Enum() : Symbol(SymbolType::Enum) {}
};