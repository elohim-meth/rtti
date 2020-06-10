#include "metaitem_p.h"

#include <rtti/metacontainer.h>

namespace rtti {

namespace {
    std::string const empty_string = {};
} // namespace

namespace internal {

    variant const &NamedVariantList::get(std::size_t index) const
    {
        std::shared_lock<std::shared_mutex> lock{m_lock};
        if (index < m_items.size())
            return m_items[index]->value;
        return variant::empty_variant;
    }

    variant const &NamedVariantList::get(std::string_view name) const
    {
        if (!name.empty())
        {
            std::shared_lock<std::shared_mutex> lock{m_lock};
            if (auto search = m_names.find(std::string{name}); search != std::end(m_names))
                if (auto index = search->second; index < m_items.size())
                    return m_items[index]->value;
        }
        return variant::empty_variant;
    }

    std::string const &NamedVariantList::name(std::size_t index) const
    {
        std::shared_lock<std::shared_mutex> lock{m_lock};
        if (index < m_items.size())
            return m_items[index]->name;
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

inline std::string const& MetaItemPrivate::qualifiedName() const
{
    if (m_qualifiedName.empty())
        m_qualifiedName = makeQualifiedName();
    return m_qualifiedName;
}

MetaItemPrivate::~MetaItemPrivate() = default;

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItem
//--------------------------------------------------------------------------------------------------------------------------------

MetaItem::MetaItem(std::string_view name, MetaContainer const &owner)
    : MetaItem(*new MetaItemPrivate{name, owner})
{}

MetaItem::MetaItem(MetaItemPrivate &value)
    : d_ptr(&value)
{}

MetaItem::~MetaItem()
{}

void MetaItem::checkDeferredDefine() const
{}

std::string const& MetaItem::name() const
{
    auto d = d_func();
    return d->name();
}

MetaContainer const* MetaItem::owner() const
{
    auto d = d_func();
    return d->owner();
}

std::string const& MetaItem::qualifiedName() const
{
    auto d = d_func();
    return d->qualifiedName();
}

std::size_t MetaItem::attributeCount() const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.size();
}

variant const& MetaItem::attribute(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.get(index);
}

std::string const& MetaItem::attributeName(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.name(index);
}

variant const& MetaItem::attribute(std::string_view name) const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_attributes.get(name);
}

void MetaItem::for_each_attribute(enum_attribute_t const &func) const
{
    if (!func)
        return;

    checkDeferredDefine();
    auto d = d_func();
    d->m_attributes.for_each([&func](std::string_view name, variant const &value) -> auto
    {
        return func(name, value);
    });
}

void MetaItem::setAttribute(std::string_view name, variant const &value)
{
    auto d = d_func();
    d->m_attributes.set(name, value);
}

void MetaItem::setAttribute(std::string_view name, variant &&value)
{
    auto d = d_func();
    d->m_attributes.set(name, std::move(value));
}

} // namespace rtti
