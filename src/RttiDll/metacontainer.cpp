#include "metacontainer_p.h"
#include "metaenum.h"
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
                          [value] (const item_t &item) {
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

inline MetaItem* MetaItemList::get(std::size_t index) const noexcept
{
    std::lock_guard<std::mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].get();
    return nullptr;
}

inline MetaItem* MetaItemList::get(const char *name) const
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

std::size_t MetaItemList::size() const noexcept
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

MetaContainer::MetaContainer(const char *name, const MetaContainer &owner)
    : MetaItem(*new MetaContainerPrivate(name, owner))
{}

MetaContainer::MetaContainer(MetaContainerPrivate &value)
    : MetaItem(value)
{}

bool MetaContainer::addItem(MetaItem *value)
{
    auto d = d_func();
    return d->addItem(value);
}

void MetaContainer::setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback)
{
    auto d = d_func();
    assert(!d->m_deferredDefine);
    if (d->m_deferredDefine)
        return;
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

std::size_t MetaContainer::count(MetaCategory category) const noexcept
{
    checkDeferredDefine();
    auto d = d_func();
    return d->size(category);
}

const MetaItem* MetaContainer::item(MetaCategory category, const char *name) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->item(category, name);
}

const MetaItem* MetaContainer::item(MetaCategory category, std::size_t index) const noexcept
{
    checkDeferredDefine();
    auto d = d_func();
    return d->item(category, index);
}

const MetaNamespace* MetaContainer::getNamespace(const char *name) const
{
    return static_cast<const MetaNamespace*>(item(mcatNamespace, name));
}

std::size_t MetaContainer::namespaceCount() const noexcept
{
    return count(mcatNamespace);
}

const MetaNamespace *MetaContainer::getNamespace(std::size_t index) const noexcept
{
    return static_cast<const MetaNamespace*>(item(mcatNamespace, index));
}

const MetaClass* MetaContainer::getClass(const char *name) const
{
    return static_cast<const MetaClass*>(item(mcatClass, name));
}

std::size_t MetaContainer::classCount() const noexcept
{
    return count(mcatClass);
}

const MetaClass* MetaContainer::getClass(std::size_t index) const noexcept
{
    return static_cast<const MetaClass*>(item(mcatClass, index));
}

const MetaConstructor* MetaContainer::getConstructor(const char *name) const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, name));
}

std::size_t MetaContainer::constructorCount() const noexcept
{
    return count(mcatConstructor);
}

const MetaConstructor *MetaContainer::defaultConstructor() const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, "default constructor"));
}

const MetaConstructor *MetaContainer::copyConstructor() const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, "copy constructor"));
}

const MetaConstructor *MetaContainer::moveConstructor() const
{

    return static_cast<const MetaConstructor*>(item(mcatConstructor, "move constructor"));
}

void MetaContainer::for_each_class(const enum_class_t &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_classes.for_each([&func](const MetaItem *item) -> bool
    {
        return func(static_cast<const MetaClass*>(item));
    });
}

const MetaMethod* MetaContainer::getMethod(const char *name) const
{
    if (!name)
        return nullptr;
    checkDeferredDefine();

    auto d = d_func();
    auto tmp = CString{name};
    const MetaMethod *result = nullptr;
    d->m_methods.for_each([&tmp, &result](const MetaItem *item) -> bool
    {
        auto method = static_cast<const MetaMethod*>(item);
        const auto &methodName = method->name();

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

const MetaMethod *MetaContainer::getMethod(const std::string &name) const
{
    return getMethod(name.c_str());
}

std::size_t MetaContainer::methodCount() const noexcept
{
    return count(mcatMethod);
}

const MetaMethod *MetaContainer::getMethod(std::size_t index) const noexcept
{
    return static_cast<const MetaMethod*>(item(mcatMethod, index));
}

void MetaContainer::for_each_method(const enum_method_t &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_methods.for_each([&func](const MetaItem *item) -> bool
    {
        return func(static_cast<const MetaMethod*>(item));
    });
}

const MetaEnum *MetaContainer::getEnum(const char *name) const
{
    return static_cast<const MetaEnum*>(item(mcatEnum, name));
}

std::size_t MetaContainer::enumCount() const noexcept
{
    return count(mcatEnum);
}

} // namespace rtti
