#pragma once

#include <cstdint>

enum Attr : uint32_t{
    TYPEDEF = 1 << 0,
    STATIC = 1 << 1
};