#ifndef METAITEM_P_H
#define METAITEM_P_H

#include "metaitem.h"
#include "variant.h"

#include <c_string.h>

#include <mutex>
#include <vector>
#include <map>
#include <string>

namespace rtti {

namespace internal {

struct DLL_LOCAL NamedVariant
{
    template<typename T>
    NamedVariant(CString const &name, T&& value)
        : name{name.data(), name.length()},
          value{std::forward<T>(value)}
    {}

    std::string const name;
    variant value;
};

class DLL_LOCAL NamedVariantList
{
public:
    template<typename T>
    void set(char const *name, T &&value);
    variant const& get(std::size_t index) const;
    variant const& get(char const *name) const;
    std::string const& name(std::size_t index) const;

    std::size_t size() const
    {
        return m_items.size();
    }

    template<typename F>
    inline void for_each(F &&func) const;

private:
    mutable std::mutex m_lock;
    std::vector<NamedVariant> m_items;
    std::map<CString, std::size_t> m_names;
};

template<typename T>
inline void NamedVariantList::set(char const *name, T &&value)
{
    if (!name)
        return;

    std::lock_guard<std::mutex> lock{m_lock};
    auto temp = CString{name};
    auto search = m_names.find(temp);

    if (search == std::end(m_names))
    {
        auto index = m_items.size();
        m_items.emplace_back(temp, std::forward<T>(value));
        m_names.emplace(std::move(temp), index);
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
    std::lock_guard<std::mutex> lock{m_lock};
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
    explicit MetaItemPrivate(char const *name)
        : m_name(name)
    {}

    MetaItemPrivate(char const *name, MetaContainer const &owner)
        : m_name(name), m_owner(&owner)
    {}
    MetaItemPrivate(std::string &&name, MetaContainer const &owner)
        : m_name(std::move(name)), m_owner(&owner)
    {}

    virtual ~MetaItemPrivate() = default;

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
