#pragma once

#include <cstdint>

inline std::uint32_t alignTo(std::uint32_t offset, std::uint32_t align)
{
    return (offset + align - 1) & ~(align - 1);
}