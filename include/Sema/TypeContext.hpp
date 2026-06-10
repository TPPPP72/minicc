#pragma once

#include <AST/Type.hpp>
#include <vector>

class TypeContext
{
public:
    TypeContext()
    {
        m_types.push_back({TypeKind::NULLTYPE, 0});
        m_types.push_back({TypeKind::INT, 1});
    }

    TypeId getIntTypeId() const { return 1; }

    TypeId getPointerTypeId(TypeId base_id)
    {
        m_types.push_back({TypeKind::PTR, base_id});
        return m_types.size() - 1;
    }

    const Type &getType(TypeId id) const { return m_types[id]; }

    TypeId getFunctionTypeId(TypeId ret_type, const std::vector<TypeId> &params)
    {
        TypeId sig_id = static_cast<TypeId>(m_func_signatures.size());
        m_func_signatures.emplace_back(params, ret_type);

        TypeId new_type_id = static_cast<TypeId>(m_types.size());
        m_types.push_back(Type{TypeKind::FUNCTION, sig_id});

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