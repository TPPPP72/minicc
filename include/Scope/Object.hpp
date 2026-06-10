#pragma once

#include <AST/Type.hpp>
#include <cstdint>
#include <string_view>

struct Object
{
    std::string_view name;
    TypeId type_id;
    std::int32_t offset{};
};