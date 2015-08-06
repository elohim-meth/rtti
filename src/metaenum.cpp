#include "metaenum_p.h"
#include "metacontainer_p.h"

namespace rtti {

MetaEnum::MetaEnum(const char *name, const MetaContainer &owner, MetaType_ID typeId)
    : MetaItem{*new MetaEnumPrivate{name, owner, typeId}}
{}

MetaEnum *MetaEnum::create(const char *name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaEnum*>(owner.getEnum(name));
    if (!result)
    {
        result = new MetaEnum(name, owner, typeId);
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

void MetaEnum::addElement(const char *name, variant &&value)
{
    auto d = d_func();
    d->m_elements.set(name, std::move(value));
}

MetaCategory MetaEnum::category() const
{
    return mcatEnum;
}

std::size_t MetaEnum::elementCount() const
{
    auto d = d_func();
    return d->m_elements.size();
}

const variant& MetaEnum::element(std::size_t index) const
{
    auto d = d_func();
    return d->m_elements.get(index);
}

const std::string &MetaEnum::elementName(std::size_t index) const
{
    auto d = d_func();
    return d->m_elements.name(index);
}

const variant& MetaEnum::element(const char *name) const
{
    auto d = d_func();
    return d->m_elements.get(name);
}

void MetaEnum::for_each_element(const enum_element_t &func) const
{
    if (!func)
        return;

    auto d = d_func();
    d->m_elements.for_each([&func](const std::string &name, const variant &value) -> bool
    {
        return func(name, value);
    });
}

} // namespace rtti
