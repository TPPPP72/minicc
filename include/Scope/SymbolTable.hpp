#include "AST/Type.hpp"
#include <Infra/Arena.hpp>
#include <Scope/Variable.hpp>
#include <string_view>
#include <unordered_map>
#include <vector>

class SymbolTable
{
public:
    SymbolTable(Arena &arena) : m_arena(arena)
    {
        m_global_scope  = m_arena.alloc<Scope>();
        m_current_scope = m_global_scope;
    }

    void enterScope()
    {
        auto *new_scope   = m_arena.alloc<Scope>();
        new_scope->parent = m_current_scope;
        m_current_scope->children.push_back(new_scope);
        m_current_scope = new_scope;
    }

    void leaveScope()
    {
        if (m_current_scope->parent)
            m_current_scope = m_current_scope->parent;
    }

    bool insertIdent(std::string_view name, Symbol *sym)
    {
        auto &idents = m_current_scope->idents;
        if (idents.find(name) != idents.end())
            return false;
        idents[name] = sym;
        return true;
    }

    bool insertTag(std::string_view name, TypeId tid)
    {
        auto &tags = m_current_scope->tags;
        if (tags.find(name) != tags.end())
            return false;
        tags[name] = tid;
        return true;
    }

    void insertGlobalIdent(std::string_view name, Symbol *sym)
    {
        m_global_scope->idents[name] = sym;
    }

    Symbol *lookupIdent(std::string_view name) const
    {
        for (const Scope *sc = m_current_scope; sc != nullptr; sc = sc->parent)
        {
            auto it = sc->idents.find(name);
            if (it != sc->idents.end())
                return it->second;
        }
        return nullptr;
    }

    TypeId lookupTag(std::string_view name) const
    {
        for (const Scope *sc = m_current_scope; sc != nullptr; sc = sc->parent)
        {
            auto it = sc->tags.find(name);
            if (it != sc->tags.end())
                return it->second;
        }
        return -1;
    }

    void registerLocal(Variable *var)
    {
        if (m_current_scope != m_global_scope)
            m_current_scope->locals.push_back(var);
    }

    std::vector<Variable *> collectLocalsFromCurrentTree() const
    {
        std::vector<Variable *> all_locals;
        collectLocalsRecursive(m_current_scope, all_locals);
        return all_locals;
    }

    std::vector<Symbol *> getGlobalSymbols() const
    {
        std::vector<Symbol *> globals;
        for (const auto &[name, sym] : m_global_scope->idents)
        {
            globals.push_back(sym);
        }
        return globals;
    }

private:
    struct Scope
    {
        std::unordered_map<std::string_view, Symbol *> idents;
        std::unordered_map<std::string_view, TypeId> tags;

        std::vector<Variable *> locals;
        Scope *parent = nullptr;
        std::vector<Scope *> children;
    };

private:
    void collectLocalsRecursive(const Scope *sc, std::vector<Variable *> &out) const
    {
        if (!sc)
            return;
        out.insert(out.end(), sc->locals.begin(), sc->locals.end());
        for (const auto *child : sc->children)
            collectLocalsRecursive(child, out);
    }

private:
    Arena &m_arena;
    Scope *m_global_scope  = nullptr;
    Scope *m_current_scope = nullptr;
};