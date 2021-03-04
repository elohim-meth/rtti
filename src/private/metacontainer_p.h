#ifndef METACONTAINER_P_H
#define METACONTAINER_P_H

#include <atomic>

#include "metaitem_p.h"
#include <rtti/metacontainer.h>

namespace rtti {
namespace internal {

class RTTI_PRIVATE MetaItemList
{
public:
    // Need custom deleter since MetaItem destructor is protected
    struct MetaItemDeleter
    {
        void operator()(MetaItem *value) const
        { delete value; }
    };

    using item_t = std::unique_ptr<MetaItem, MetaItemDeleter>;

    bool add(MetaItem *value);
    MetaItem* get(std::size_t index) const;
    MetaItem* get(std::string_view name) const;
    std::size_t size() const;
    template<typename F> void for_each(F &&func) const;

private:
    mutable std::shared_mutex m_lock;
    std::vector<item_t> m_items;
    std::unordered_map<std::string_view, std::size_t> m_names;
};

template<typename F>
inline void MetaItemList::for_each(F &&func) const
{
    std::shared_lock lock{m_lock};
    for(auto &item: m_items)
    {
        if (!func(item.get()))
            break;
    }
}

} // namespace internal

class RTTI_PRIVATE MetaContainerPrivate: public MetaItemPrivate
{
public:
    using MetaItemPrivate::MetaItemPrivate;

    MetaItem* item(MetaCategory category, std::size_t index) const
    { return m_lists[category]->get(index); }
    std::size_t size(MetaCategory category) const
    { return m_lists[category]->size(); }
    MetaItem* item(MetaCategory category, std::string_view name) const
    { return m_lists[category]->get(name); }
protected:
    bool addItem(MetaItem *value);
    MetaItem* findMethod(MetaCategory category, std::string_view name) const;

private:
    internal::MetaItemList m_namespaces;
    internal::MetaItemList m_classes;
    internal::MetaItemList m_properties;
    internal::MetaItemList m_methods;
    internal::MetaItemList m_enums;
    internal::MetaItemList m_constructors;
    std::array<internal::MetaItemList*, mcatCount> const m_lists = {
        {&m_namespaces, &m_classes, &m_properties,
         &m_methods, &m_enums, &m_constructors}
    };

    mutable std::atomic_bool m_deferredDefine_lock = false;
    mutable std::unique_ptr<IDefinitionCallbackHolder> m_deferredDefine;

    friend class rtti::MetaContainer;
};

} // namespace rtti


#endif // METACONTAINER_P_H

