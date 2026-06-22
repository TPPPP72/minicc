#pragma once

#include <cstdint>
#include <vector>
#include <string_view>

enum class TypeKind : std::uint32_t
{
    CHAR,
    SHORT,
    INT,
    LONG,
    PTR,
    FUNCTION,
    ARRAY,
    STRUCT,
    UNION
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

struct alignas(32) Member
{
    std::string_view name;
    TypeId type_id;
    std::uint32_t offset;
};

struct alignas(32) Layout{
    std::vector<Member> members;
    std::uint32_t align{};
};