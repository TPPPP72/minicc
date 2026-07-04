#pragma once

#include <Scope/Symbol.hpp>

struct Typedef : Symbol
{
    Typedef() : Symbol(SymbolType::Typedef) {}
};