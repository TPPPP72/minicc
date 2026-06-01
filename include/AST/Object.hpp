#pragma once

#include <cstdint>
#include <string_view>

struct Object
{
    Object *next;
    std::string_view name;
    std::int32_t offset;
};