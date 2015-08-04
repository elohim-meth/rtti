﻿#ifndef METACONTAINER_P_H
#define METACONTAINER_P_H

#include "metaitem_p.h"
#include "metacontainer.h"

namespace rtti {
namespace internal {

struct DLL_LOCAL MetaContainerAccess: MetaContainer
{
    bool addItem(MetaItem *value)
    { return MetaContainer::addItem(value); }

    friend class rtti::MetaNamespace;
    friend class rtti::MetaClass;
    friend class rtti::MetaConstructor;
    friend class rtti::MetaProperty;
    friend class rtti::MetaMethod;
    friend class rtti::MetaEnum;
};

class DLL_LOCAL MetaItemList
{
public:
    // Need custom deleter since MetaItem destructor is protected
    struct MetaItemDeleter {
        void operator()(MetaItem *value) const
        { delete value; }
    };

    using item_t = std::unique_ptr<MetaItem, MetaItemDeleter>;

    bool add(MetaItem *value);
    MetaItem* get(std::size_t index) const noexcept;
    MetaItem* get(const char *name) const;
    std::size_t size() const noexcept;
    template<typename F> void for_each(F &&func) const;

private:
    mutable std::mutex m_lock;
    std::vector<item_t> m_items;
    std::map<CString, std::size_t> m_names;
};

template<typename F>
inline void MetaItemList::for_each(F &&func) const
{
    std::lock_guard<std::mutex> lock{m_lock};
    for(auto &item: m_items)
    {
        if (!func(item.get()))
            break;
    }
}

} // namespace internal

class DLL_LOCAL MetaContainerPrivate: public MetaItemPrivate
{
public:
    using MetaItemPrivate::MetaItemPrivate;

    MetaItem* item(MetaCategory category, std::size_t index) const noexcept
    { return m_lists[category]->get(index); }
    std::size_t size(MetaCategory category) const noexcept
    { return m_lists[category]->size(); }
    MetaItem* item(MetaCategory category, const char *name) const
    { return m_lists[category]->get(name); }

protected:
    bool addItem(MetaItem *value);

private:
    internal::MetaItemList m_namespaces;
    internal::MetaItemList m_classes;
    internal::MetaItemList m_properties;
    internal::MetaItemList m_methods;
    internal::MetaItemList m_enums;
    internal::MetaItemList m_constructors;
    const std::array<internal::MetaItemList*, mcatCount> m_lists = {
        {&m_namespaces, &m_classes, &m_properties,
         &m_methods, &m_enums, &m_constructors}
    };

    mutable bool m_deferredDefine_lock = false;
    mutable std::unique_ptr<IDefinitionCallbackHolder> m_deferredDefine;

    friend class rtti::MetaContainer;
};

} // namespace rtti


#endif // METACONTAINER_P_H
