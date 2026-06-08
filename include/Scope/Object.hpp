#pragma once

#include <AST/Type.hpp>
#include <cstdint>
#include <string_view>

struct Object
{
    Object *next;
    std::string_view name;
    std::int32_t offset;
    TypeId type_id;
};