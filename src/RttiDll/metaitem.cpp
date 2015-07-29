#include "metaitem_p.h"
#include "metacontainer.h"

namespace rtti {

namespace {
const std::string empty_string = {};
}

namespace internal {

const variant& NamedVariantList::get(std::size_t index) const noexcept
{
    std::lock_guard<std::mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].value;
    return variant::empty_variant;
}

const variant& NamedVariantList::get(const char *name) const
{
    if (name)
    {
        std::lock_guard<std::mutex> lock{m_lock};
        auto temp = CString{name};
        auto search = m_names.find(temp);
        if (search != std::end(m_names))
        {
            auto index = search->second;
            if (index < m_items.size())
                return m_items[index].value;
        }
    }
    return variant::empty_variant;
}

const std::string& NamedVariantList::name(std::size_t index) const noexcept
{
    std::lock_guard<std::mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].name;
    return empty_string;
}

} // namespace internal

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItemPrivate
//--------------------------------------------------------------------------------------------------------------------------------

inline std::string MetaItemPrivate::makeQualifiedName() const
{
    std::string result = m_owner ? m_owner->qualifiedName() : "";
    return std::move(result) + (m_owner ? "::" + m_name : "");
}

inline const std::string& MetaItemPrivate::qualifiedName() const
{
    if (m_qualifiedName.empty())
        m_qualifiedName = makeQualifiedName();
    return m_qualifiedName;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItem
//--------------------------------------------------------------------------------------------------------------------------------

MetaItem::MetaItem(const char *name, const MetaContainer &owner)
    : d_ptr(new MetaItemPrivate(name, owner))
{}

MetaItem::MetaItem(MetaItemPrivate &value)
    : d_ptr(&value)
{}

MetaItem::~MetaItem()
{}

void MetaItem::checkDeferredDefine() const
{}

const std::string& MetaItem::name() const
{
    auto d = d_func();
    return d->name();
}

const MetaContainer *MetaItem::owner() const noexcept
{
    auto d = d_func();
    return d->owner();
}

const std::string& MetaItem::qualifiedName() const
{
    auto d = d_func();
    return d->qualifiedName();
}

std::size_t MetaItem::attributeCount() const noexcept
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.size();
}

const variant& MetaItem::attribute(std::size_t index) const noexcept
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.get(index);
}

const std::string &MetaItem::attributeName(std::size_t index) const noexcept
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.name(index);
}

const variant& MetaItem::attribute(const char *name) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.get(name);
}

void MetaItem::for_each_attribute(const enum_attribute_t &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_attributes.for_each([&func](const std::string &name, const variant &value) -> bool
    {
        return func(name, value);
    });
}

void MetaItem::setAttribute(const char *name, const variant &value)
{
    auto d = d_func();
    d->m_attributes.set(name, value);
}

void MetaItem::setAttribute(const char *name, variant &&value)
{
    auto d = d_func();
    d->m_attributes.set(name, std::move(value));
}


//--------------------------------------------------------------------------------------------------------------------------------
// MetaProperty
//--------------------------------------------------------------------------------------------------------------------------------

MetaCategory MetaProperty::category() const noexcept
{
    return mcatProperty;
}


} // namespace rtti
