#pragma once

#include <cstdint>
#include <vector>

enum class TypeKind : std::uint32_t
{
    NULLTYPE,
    INT,
    PTR,
    FUNCTION
};

using TypeId = std::uint32_t;

struct alignas(8) Type
{
    TypeKind kind;
    TypeId base_type_id{};
};

struct alignas(32) FunctionSignature
{
    std::vector<TypeId> param_types;
    TypeId return_type_id;
};