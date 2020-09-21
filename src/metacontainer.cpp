#include "metacontainer_p.h"

#include <rtti/metaenum.h>
#include <rtti/metaconstructor.h>
#include <rtti/metaclass.h>
#include <rtti/metanamespace.h>

#include <rtti/finally.h>

#include <algorithm>
#include <cassert>

namespace rtti {

namespace internal {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItemList
//--------------------------------------------------------------------------------------------------------------------------------

bool MetaItemList::add(MetaItem *value)
{
    if (!value)
        return false;

    std::unique_lock<std::shared_mutex> lock{m_lock};
    auto itemFound = (std::find_if(
                          std::cbegin(m_items), std::cend(m_items),
                          [value] (item_t const &item) {
                              return value == item.get();
                          }) != std::end(m_items));

    auto &name = value->name();
    auto nameFound = (m_names.find(name) != std::end(m_names));

    if (!itemFound && !nameFound)
    {
        auto index = m_items.size();
        m_items.emplace_back(value);
        m_names.emplace(name, index);
        return true;
    }
    return false;
}

inline MetaItem* MetaItemList::get(std::size_t index) const
{
    std::shared_lock<std::shared_mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].get();
    return nullptr;
}

inline MetaItem* MetaItemList::get(std::string_view name) const
{
    if (name.empty())
        return nullptr;

    std::shared_lock<std::shared_mutex> lock{m_lock};
    if (auto it = m_names.find(name); it != std::end(m_names))
        if (auto index = it->second; index < m_items.size())
            return m_items[index].get();

    return nullptr;
}

std::size_t MetaItemList::size() const
{
    std::shared_lock<std::shared_mutex> lock{m_lock};
    return m_items.size();
}

} // namespace internal

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainerPrivate
//--------------------------------------------------------------------------------------------------------------------------------

inline bool MetaContainerPrivate::addItem(MetaItem *value)
{
    if (!value)
        return false;

    auto category = value->category();
    return m_lists[category]->add(value);
}

MetaItem* MetaContainerPrivate::findMethod(MetaCategory category, std::string_view name) const
{
    MetaItem *result = nullptr;
    m_lists[category]->for_each([&name, &result](MetaItem *item) -> bool
    {
        auto const &methodName = item->name();

        if (auto pos = methodName.find('(');
            pos != std::string::npos)
        {
            auto length = std::min(pos, name.size());
            if (auto cmp = methodName.compare(0, pos, name, 0, length);
                cmp == 0)
            {
                 if (pos == name.size())
                 {
                     result = item;
                     return false;
                 }

                 cmp = methodName.compare(pos, methodName.size() - pos,
                                          name, pos, name.size() - pos);
                 if (cmp == 0)
                 {
                     result = item;
                     return false;
                 }
            }
        }
        else if (auto cmp = methodName.compare(name);
                 cmp == 0)
        {
            result = item;
            return false;
        }
        return true;
    });
    return result;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainer
//--------------------------------------------------------------------------------------------------------------------------------

MetaContainer::MetaContainer(std::string_view name, MetaContainer const &owner)
    : MetaItem(*new MetaContainerPrivate(name, owner))
{}

MetaContainer::MetaContainer(MetaContainerPrivate &value)
    : MetaItem(value)
{}

//--------------------------------------------------------------------------------------------------------------------------------
// Lazy definition
//--------------------------------------------------------------------------------------------------------------------------------

void MetaContainer::setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback)
{
    auto d = d_func();
    if (d->m_deferredDefine)
        throw definition_error{"Metacontainer " + qualifiedName() +
                               " already has deferred definition"};
    d->m_deferredDefine = std::move(callback);
}

void MetaContainer::checkDeferredDefine() const
{
    auto d = d_func();
    if (!d->m_deferredDefine)
        return;
    if (d->m_deferredDefine_lock.exchange(true, std::memory_order_acquire))
        return;

    FINALLY{
        d->m_deferredDefine = nullptr;
        d->m_deferredDefine_lock.store(false, std::memory_order_release);
    };

    d->m_deferredDefine->invoke(*const_cast<MetaContainer*>(this));
}

void MetaContainer::forceDeferredDefine(ForceDeferred type) const
{
    if (type == ForceDeferred::SelfOnly)
        checkDeferredDefine();
    else if (type == ForceDeferred::Recursive)
    {
        checkDeferredDefine();
        auto d = d_func();
        d->m_classes.for_each([](MetaItem *item)
        {
            auto *c = static_cast<MetaClass const*>(item);
            c->forceDeferredDefine(ForceDeferred::Recursive);
            return true;
        });

        d->m_namespaces.for_each([](MetaItem const *item)
        {
            auto *ns = static_cast<MetaContainer const*>(item);
            ns->forceDeferredDefine(ForceDeferred::Recursive);
            return true;
        });
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
// Common items
//--------------------------------------------------------------------------------------------------------------------------------

bool MetaContainer::addItem(MetaItem *value)
{
    auto d = d_func();
    return d->addItem(value);
}

std::size_t MetaContainer::count(MetaCategory category) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->size(category);
}

MetaItem const* MetaContainer::item(MetaCategory category, std::string_view name) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->item(category, name);
}

MetaItem const* MetaContainer::item(MetaCategory category, std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->item(category, index);
}

//--------------------------------------------------------------------------------------------------------------------------------
// Namespace
//--------------------------------------------------------------------------------------------------------------------------------

MetaNamespace const* MetaContainer::getNamespace(std::string_view name) const
{
    return static_cast<MetaNamespace const*>(item(mcatNamespace, name));
}

std::size_t MetaContainer::namespaceCount() const
{
    return count(mcatNamespace);
}

MetaNamespace const* MetaContainer::getNamespace(std::size_t index) const
{
    return static_cast<MetaNamespace const*>(item(mcatNamespace, index));
}

//--------------------------------------------------------------------------------------------------------------------------------
// Class
//--------------------------------------------------------------------------------------------------------------------------------

MetaClass const* MetaContainer::getClass(std::string_view name) const
{
    return static_cast<MetaClass const*>(item(mcatClass, name));
}

std::size_t MetaContainer::classCount() const
{
    return count(mcatClass);
}

MetaClass const* MetaContainer::getClass(std::size_t index) const
{
    return static_cast<MetaClass const*>(item(mcatClass, index));
}

void MetaContainer::for_each_class(enum_class_t const &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_classes.for_each([&func](MetaItem const *item) -> bool
    {
        return func(static_cast<MetaClass const*>(item));
    });
}

//--------------------------------------------------------------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------------------------------------------------------------

MetaConstructor const* MetaContainer::getConstructor(std::string_view name) const
{
    checkDeferredDefine();
    return static_cast<MetaConstructor const*>(d_func()->findMethod(mcatConstructor, name));
}

std::size_t MetaContainer::constructorCount() const
{
    return count(mcatConstructor);
}

MetaConstructor const* MetaContainer::getConstructor(std::size_t index) const
{
    return static_cast<MetaConstructor const*>(item(mcatConstructor, index));
}

MetaConstructor const* MetaContainer::defaultConstructor() const
{
    return getConstructor(DEFAULT_CONSTRUCTOR_SIG);
}

MetaConstructor const* MetaContainer::copyConstructor() const
{
    return getConstructor(COPY_CONSTRUCTOR_SIG);
}

MetaConstructor const* MetaContainer::moveConstructor() const
{

    return getConstructor(MOVE_CONSTRUCTOR_SIG);
}

//--------------------------------------------------------------------------------------------------------------------------------
// Method
//--------------------------------------------------------------------------------------------------------------------------------

MetaMethod const* MetaContainer::getMethodInternal(std::string_view name) const
{
    checkDeferredDefine();
    return static_cast<MetaMethod const*>(d_func()->findMethod(mcatMethod, name));
}

MetaMethod const* MetaContainer::getMethod(std::string_view name) const
{
    if (name.empty())
        return nullptr;
    return getMethodInternal(name);
}

std::size_t MetaContainer::methodCount() const
{
    return count(mcatMethod);
}

MetaMethod const* MetaContainer::getMethod(std::size_t index) const
{
    return static_cast<MetaMethod const*>(item(mcatMethod, index));
}

void MetaContainer::for_each_method(enum_method_t const &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_methods.for_each([&func](MetaItem const *item) -> bool
    {
        return func(static_cast<MetaMethod const*>(item));
    });
}

//--------------------------------------------------------------------------------------------------------------------------------
// Property
//--------------------------------------------------------------------------------------------------------------------------------

MetaProperty const* MetaContainer::getPropertyInternal(std::string_view name) const
{
    return static_cast<MetaProperty const*>(item(mcatProperty, name));
}

MetaProperty const* MetaContainer::getProperty(std::string_view name) const
{
    return getPropertyInternal(name);
}

std::size_t MetaContainer::propertyCount() const
{
    return count(mcatProperty);
}

MetaProperty const* MetaContainer::getProperty(std::size_t index) const
{
    return static_cast<MetaProperty const*>(item(mcatProperty, index));
}

//--------------------------------------------------------------------------------------------------------------------------------
// Enum
//--------------------------------------------------------------------------------------------------------------------------------

MetaEnum const* MetaContainer::getEnum(std::string_view name) const
{
    return static_cast<MetaEnum const*>(item(mcatEnum, name));
}

std::size_t MetaContainer::enumCount() const
{
    return count(mcatEnum);
}

MetaEnum const* MetaContainer::getEnum(std::size_t index) const
{
    return static_cast<MetaEnum const*>(item(mcatEnum, index));
}

} // namespace rtti
