#pragma once

#include <cstdint>
#include <vector>
#include <string_view>

enum class TypeKind : std::uint32_t
{
    NULLTYPE,
    INT,
    PTR,
    FUNCTION,
    ARRAY
};

using TypeId = std::uint32_t;

struct alignas(16) Type
{
    TypeKind kind;
    TypeId base_type_id{};
    std::uint32_t size;
};

struct alignas(64) FunctionSignature
{
    std::vector<TypeId> param_types;
    std::vector<std::string_view> param_names;
    TypeId return_type_id;
};