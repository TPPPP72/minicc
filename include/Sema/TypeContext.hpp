#pragma once

#include <AST/Type.hpp>
#include <vector>

class TypeContext
{
public:
    TypeContext()
    {
        types.push_back({TypeKind::NULLTYPE, 0});
        types.push_back({TypeKind::INT, 1});
    }

    TypeId getIntTypeId() const { return 1; }

    TypeId getPointerTypeId(TypeId base_id)
    {
        types.push_back({TypeKind::PTR, base_id});
        return types.size() - 1;
    }

    const Type &getType(TypeId id) const { return types[id]; }

private:
    std::vector<Type> types;
};