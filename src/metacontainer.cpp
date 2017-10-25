#include "metacontainer_p.h"
#include "metaenum.h"
#include "metamethod.h"
#include "metaproperty.h"
#include "metaconstructor.h"
#include "metaclass.h"
#include "metanamespace.h"

#include <finally.h>

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

    std::lock_guard<std::mutex> lock{m_lock};
    auto itemFound = (std::find_if(
                          std::begin(m_items), std::end(m_items),
                          [value] (item_t const &item) {
                              return value == item.get();
                          }) != std::end(m_items));
    auto name = CString{value->name()};
    auto nameFound = (m_names.find(name) != std::end(m_names));

    if (!itemFound && !nameFound) {
        auto index = m_items.size();
        m_items.emplace_back(value);
        m_names.emplace(std::move(name), index);
        return true;
    }
    return false;
}

inline MetaItem* MetaItemList::get(std::size_t index) const
{
    std::lock_guard<std::mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].get();
    return nullptr;
}

inline MetaItem* MetaItemList::get(char const *name) const
{
    if (!name)
        return nullptr;

    std::lock_guard<std::mutex> lock{m_lock};
    auto it = m_names.find(CString{name});
    if (it != std::end(m_names)) {
        auto index = it->second;
        if (index < m_items.size())
            return m_items[index].get();
    }

    return nullptr;
}

std::size_t MetaItemList::size() const
{
    std::lock_guard<std::mutex> lock{m_lock};
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

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainer
//--------------------------------------------------------------------------------------------------------------------------------

MetaContainer::MetaContainer(std::string_view const &name, MetaContainer const &owner)
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
    if (d->m_deferredDefine_lock || !d->m_deferredDefine)
        return;

    d->m_deferredDefine_lock = true;
    FINALLY{
        d->m_deferredDefine_lock = false;
        d->m_deferredDefine = nullptr;
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
            static_cast<MetaClass const*>(item)->forceDeferredDefine(ForceDeferred::Recursive);
            return true;
        });

        d->m_namespaces.for_each([](MetaItem const *item)
        {
            static_cast<MetaContainer const*>(item)->forceDeferredDefine(ForceDeferred::Recursive);
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

MetaItem const* MetaContainer::item(MetaCategory category, char const *name) const
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

MetaNamespace const* MetaContainer::getNamespace(char const *name) const
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

MetaClass const* MetaContainer::getClass(char const *name) const
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

MetaConstructor const* MetaContainer::getConstructor(char const *name) const
{
    return static_cast<MetaConstructor const*>(item(mcatConstructor, name));
}

MetaConstructor const* MetaContainer::getConstructor(std::string const &name) const
{
    return getConstructor(name.c_str());
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
    return static_cast<MetaConstructor const*>(item(mcatConstructor, "default constructor"));
}

MetaConstructor const* MetaContainer::copyConstructor() const
{
    return static_cast<MetaConstructor const*>(item(mcatConstructor, "copy constructor"));
}

MetaConstructor const* MetaContainer::moveConstructor() const
{

    return static_cast<MetaConstructor const*>(item(mcatConstructor, "move constructor"));
}

//--------------------------------------------------------------------------------------------------------------------------------
// Method
//--------------------------------------------------------------------------------------------------------------------------------

MetaMethod const* MetaContainer::getMethodInternal(char const *name) const
{
    checkDeferredDefine();

    auto d = d_func();
    auto tmp = CString{name};
    MetaMethod const *result = nullptr;
    d->m_methods.for_each([&tmp, &result](MetaItem const *item) -> bool
    {
        auto method = static_cast<MetaMethod const*>(item);
        auto const &methodName = method->name();

        auto pos = methodName.find('(');
        if (pos != std::string::npos)
        {
            auto length = std::min(pos, tmp.length());
            auto cmp = methodName.compare(0, pos, tmp.data(), length);
            if (cmp == 0)
            {
                 if (pos == tmp.length())
                 {
                     result = method;
                     return false;
                 }

                 cmp = methodName.compare(pos, methodName.size() - pos,
                                          tmp.data() + pos, tmp.length() - pos);
                 if (cmp == 0)
                 {
                     result = method;
                     return false;
                 }
            }
        }
        return true;
    });
    return result;
}

MetaMethod const* MetaContainer::getMethod(char const *name) const
{
    if (!name)
        return nullptr;
    return getMethodInternal(name);
}

MetaMethod const* MetaContainer::getMethod(std::string const &name) const
{
    return getMethod(name.c_str());
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

MetaProperty const* MetaContainer::getPropertyInternal(char const *name) const
{
    return static_cast<MetaProperty const*>(item(mcatProperty, name));
}

MetaProperty const* MetaContainer::getProperty(char const *name) const
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

MetaEnum const* MetaContainer::getEnum(char const *name) const
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
