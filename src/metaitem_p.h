#ifndef METAITEM_P_H
#define METAITEM_P_H

#include <rtti/metaitem.h>
#include <rtti/variant.h>

#include <shared_mutex>
#include <vector>
#include <map>
#include <string>

namespace rtti {

namespace internal {

    struct RTTI_PRIVATE named_variant
    {
        template<typename T>
        named_variant(std::string_view name, T &&value)
            : name{name}
            , value{std::forward<T>(value)}
        {}

        std::string const name;
        variant value;
    };

    class RTTI_PRIVATE NamedVariantList
    {
    public:
        template<typename T>
        void set(std::string_view name, T &&value);
        variant const &get(std::size_t index) const;
        variant const &get(std::string_view name) const;
        std::string const &name(std::size_t index) const;

        std::size_t size() const
        {
            std::shared_lock lock{m_lock};
            return m_items.size();
        }

        template<typename F>
        inline void for_each(F &&func) const;

    private:
        mutable std::shared_mutex m_lock;
        std::vector<std::unique_ptr<named_variant>> m_items;
        std::unordered_map<std::string_view, std::size_t> m_names;
    };

    template<typename T>
    inline void NamedVariantList::set(std::string_view name, T &&value)
    {
        if (name.empty())
            return;

        std::unique_lock lock{m_lock};
        if (auto search = m_names.find(name); search == std::end(m_names))
        {
            auto &item = m_items.emplace_back(new named_variant{name, std::forward<T>(value)});
            m_names.emplace(item->name, m_items.size() - 1);
        }
        else
        {
            auto index  = search->second;
            auto &item  = m_items.at(index);
            item->value = std::forward<T>(value);
        }
    }

    template<typename F>
    inline void NamedVariantList::for_each(F &&func) const
    {
        std::shared_lock lock{m_lock};
        for (auto const &item: m_items)
        {
            if (!func(item->name, item->value))
                break;
        }
    }

} //namespace internal

class RTTI_PRIVATE MetaItemPrivate
{
public:
    MetaItemPrivate()                                   = delete;
    MetaItemPrivate(MetaItemPrivate const &)            = delete;
    MetaItemPrivate &operator=(MetaItemPrivate const &) = delete;
    MetaItemPrivate(MetaItemPrivate &&)                 = delete;
    MetaItemPrivate &operator=(MetaItemPrivate &&)      = delete;

    // Constructor for global namespace
    explicit MetaItemPrivate(std::string_view name)
        : m_name(name)
    {}

    MetaItemPrivate(std::string_view name, MetaContainer const &owner)
        : m_name(name)
        , m_owner(&owner)
    {}

    virtual ~MetaItemPrivate();

protected:
    std::string const &name() const
    { return m_name; }
    MetaContainer const *owner() const
    { return m_owner; }
    std::string const &qualifiedName() const;

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
