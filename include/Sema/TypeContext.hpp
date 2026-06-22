#pragma once

#include <Util/Tags.hpp>
#include <AST/Type.hpp>
#include <Util/Align.hpp>
#include <string_view>
#include <vector>

class TypeContext
{
public:
    TypeContext()
    {
        m_types.emplace_back(TypeKind::INT, -1, 8, 8);
        m_types.emplace_back(TypeKind::CHAR, -1, 1, 1);
    }

    TypeId getIntTypeId() const noexcept { return 0; }

    TypeId getCharTypeId() const noexcept { return 1; }

    TypeId getPointerTypeId(TypeId base_id)
    {
        m_types.emplace_back(TypeKind::PTR, base_id, 8, 8);
        return m_types.size() - 1;
    }

    TypeId getArrayTypeId(TypeId base_id, std::uint32_t len)
    {
        auto type = getType(base_id);
        m_types.emplace_back(TypeKind::ARRAY, base_id, len * type.size, type.align);
        return m_types.size() - 1;
    }

    const Type &getType(TypeId id) const { return m_types[id]; }

    TypeId getFunctionTypeId(TypeId ret_type, const std::vector<TypeId> &param_types, const std::vector<std::string_view> &param_names)
    {
        TypeId sig_id = static_cast<TypeId>(m_func_signatures.size());
        m_func_signatures.emplace_back(param_types, param_names, ret_type);

        TypeId new_type_id = static_cast<TypeId>(m_types.size());
        m_types.emplace_back(TypeKind::FUNCTION, sig_id);

        return new_type_id;
    }

    const FunctionSignature &getFuncSignature(TypeId tid) const
    {
        return m_func_signatures[m_types[tid].base_type_id];
    }

    TypeId getStructTypeId(const std::vector<std::pair<std::string_view, TypeId>> &raw_members)
    {
        return getLayoutTypeId<IsStruct>(raw_members);
    }

    TypeId getUnionTypeId(const std::vector<std::pair<std::string_view, TypeId>> &raw_members)
    {
        return getLayoutTypeId<IsUnion>(raw_members);
    }

    const Layout &getLayout(TypeId tid) const
    {
        return m_layouts[m_types[tid].base_type_id];
    }

private:
    template <typename T>
    TypeId getLayoutTypeId(const std::vector<std::pair<std::string_view, TypeId>> &raw_members)
    {
        TypeId layout_id = static_cast<TypeId>(m_layouts.size());
        auto &layout     = m_layouts.emplace_back();

        std::uint32_t current_align  = 1;
        std::uint32_t current_offset = 0;

        if constexpr (std::is_same_v<T, IsStruct>)
        {
            for (const auto &[name, member_type_id] : raw_members)
            {
                auto member_type = getType(member_type_id);
                current_offset   = alignTo(current_offset, member_type.align);
                layout.members.emplace_back(name, member_type_id, current_offset);
                current_offset += member_type.size;

                if (current_align < member_type.align)
                    current_align = member_type.align;
            }
        }
        else
        {
            for (const auto &[name, member_type_id] : raw_members)
            {
                layout.members.emplace_back(name, member_type_id, 0);
                auto member_type = getType(member_type_id);
                if (current_align < member_type.align)
                    current_align = member_type.align;
                if (current_offset < member_type.size)
                    current_offset = member_type.size;
            }
        }

        TypeId new_type_id = static_cast<TypeId>(m_types.size());

        if constexpr (std::is_same_v<T, IsStruct>)
            m_types.emplace_back(TypeKind::STRUCT, layout_id, alignTo(current_offset, current_align), current_align);
        else
            m_types.emplace_back(TypeKind::UNION, layout_id, alignTo(current_offset, current_align), current_align);

        return new_type_id;
    }

private:
    std::vector<Type> m_types;
    std::vector<FunctionSignature> m_func_signatures;
    std::vector<Layout> m_layouts;
};