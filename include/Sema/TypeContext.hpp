#pragma once

#include <AST/Type.hpp>
#include <string_view>
#include <vector>

class TypeContext
{
public:
    TypeContext()
    {
        m_types.emplace_back(TypeKind::NULLTYPE, 0, 0);
        m_types.emplace_back(TypeKind::INT, 0, 8);
        m_types.emplace_back(TypeKind::CHAR, 0, 1);
    }

    TypeId getIntTypeId() const noexcept { return 1; }

    TypeId getCharTypeId() const noexcept { return 2; }

    TypeId getPointerTypeId(TypeId base_id)
    {
        m_types.emplace_back(TypeKind::PTR, base_id, 8);
        return m_types.size() - 1;
    }

    TypeId getArrayTypeId(TypeId base_id, std::uint32_t len)
    {
        auto size = getType(base_id).size;
        m_types.emplace_back(TypeKind::ARRAY, base_id, len * size);
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

private:
    std::vector<Type> m_types;
    std::vector<FunctionSignature> m_func_signatures;
};