#pragma once

#include <cstdint>
#include <vector>
#include <string_view>

enum class TypeKind : std::uint32_t
{
    INT,
    CHAR,
    PTR,
    FUNCTION,
    ARRAY,
    STRUCT
};

using TypeId = std::int32_t;

struct alignas(16) Type
{
    TypeKind kind;
    TypeId base_type_id{-1};
    std::uint32_t size;
    std::uint32_t align{};
};

struct alignas(64) FunctionSignature
{
    std::vector<TypeId> param_types;
    std::vector<std::string_view> param_names;
    TypeId return_type_id;
};

struct alignas(32) StructMember
{
    std::string_view name;
    TypeId type_id;
    std::uint32_t offset;
};

struct alignas(32) StructLayout{
    std::vector<StructMember> members;
    std::uint32_t align{};
};