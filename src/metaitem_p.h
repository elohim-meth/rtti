#ifndef METAITEM_P_H
#define METAITEM_P_H

#include "metaitem.h"
#include "variant.h"

#include <shared_mutex>
#include <vector>
#include <map>
#include <string>

namespace rtti {

namespace internal {

struct DLL_LOCAL NamedVariant
{
    template<typename T>
    NamedVariant(std::string_view const &name, T&& value)
        : name{name}, value{std::forward<T>(value)}
    {}

    std::string const name;
    variant value;
};

class DLL_LOCAL NamedVariantList
{
public:
    template<typename T>
    void set(std::string_view const &name, T &&value);
    variant const& get(std::size_t index) const;
    variant const& get(std::string_view const &name) const;
    std::string const& name(std::size_t index) const;

    std::size_t size() const
    {
        std::shared_lock<std::shared_mutex> lock{m_lock};
        return m_items.size();
    }

    template<typename F>
    inline void for_each(F &&func) const;

private:
    mutable std::shared_mutex m_lock;
    std::vector<NamedVariant> m_items;
    std::unordered_map<std::string_view, std::size_t> m_names;
};

template<typename T>
inline void NamedVariantList::set(std::string_view const &name, T &&value)
{
    if (name.empty())
        return;

    std::unique_lock<std::shared_mutex> lock{m_lock};
    if (auto search = m_names.find(name); search == std::end(m_names))
    {
        auto const &item = m_items.emplace_back(name, std::forward<T>(value));
        m_names.emplace(item.name, m_items.size() - 1);
    }
    else
    {
        auto index = search->second;
        m_items.at(index).value = std::forward<T>(value);
    }
}

template<typename F>
inline void NamedVariantList::for_each(F &&func) const
{
    std::shared_lock<std::shared_mutex> lock{m_lock};
    for(auto const &item: m_items)
    {
        if (!func(item.name, item.value))
            break;
    }
}

} //namespace internal

class DLL_LOCAL MetaItemPrivate
{
public:
    MetaItemPrivate() = delete;
    MetaItemPrivate(MetaItemPrivate const&) = delete;
    MetaItemPrivate& operator=(MetaItemPrivate const&) = delete;
    MetaItemPrivate(MetaItemPrivate&&) = delete;
    MetaItemPrivate& operator=(MetaItemPrivate&&) = delete;

    // Constructor for global namespace
    explicit MetaItemPrivate(std::string_view const &name)
        : m_name(name)
    {}

    MetaItemPrivate(std::string_view const &name, MetaContainer const &owner)
        : m_name(name), m_owner(&owner)
    {}

    virtual ~MetaItemPrivate();

protected:
    std::string const& name() const
    { return m_name; }
    MetaContainer const* owner() const
    { return m_owner; }
    std::string const& qualifiedName() const;

private:
    std::string makeQualifiedName() const;

    std::string const m_name;
    MetaContainer const *m_owner = nullptr;
    internal::NamedVariantList m_attributes;
    mutable std::string m_qualifiedName = {};

    friend class rtti::MetaItem;
};

} //namespace rtti

#endif // METAITEM_P_H
