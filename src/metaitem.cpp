#include "metaitem_p.h"
#include "metacontainer.h"

namespace rtti {

namespace {
std::string const empty_string = {};
}

namespace internal {

variant const& NamedVariantList::get(std::size_t index) const
{
    std::lock_guard<std::mutex> lock{m_lock};
    if (index < m_items.size())
        return m_items[index].value;
    return variant::empty_variant;
}

variant const& NamedVariantList::get(char const *name) const
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

std::string const& NamedVariantList::name(std::size_t index) const
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

MetaItem::MetaItem(char const *name, MetaContainer const &owner)
    : d_ptr(new MetaItemPrivate{name, owner})
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

variant const& MetaItem::attribute(char const *name) const
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
    d->m_attributes.for_each([&func](std::string const &name, variant const &value) -> bool
    {
        return func(name, value);
    });
}

void MetaItem::setAttribute(char const *name, variant const &value)
{
    auto d = d_func();
    d->m_attributes.set(name, value);
}

void MetaItem::setAttribute(char const *name, variant &&value)
{
    auto d = d_func();
    d->m_attributes.set(name, std::move(value));
}

} // namespace rtti
