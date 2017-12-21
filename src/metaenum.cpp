#include "metaenum_p.h"
#include "metacontainer_p.h"

namespace rtti {

MetaEnum::MetaEnum(std::string_view const &name, const MetaContainer &owner, MetaType_ID typeId)
    : MetaItem{*new MetaEnumPrivate{name, owner, typeId}}
{}

MetaEnum *MetaEnum::create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaEnum*>(owner.getEnum(name));
    if (!result)
    {
        result = new MetaEnum{name, owner, typeId};
        INVOKE_PROTECTED(owner, addItem, result);
    }
    return result;
}

void MetaEnum::addElement(std::string_view const &name, variant &&value)
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

const variant& MetaEnum::element(std::string_view const &name) const
{
    auto d = d_func();
    return d->m_elements.get(name);
}

void MetaEnum::for_each_element(enum_element_t const &func) const
{
    if (!func)
        return;

    auto d = d_func();
    d->m_elements.for_each([&func](std::string const &name, variant const &value) -> bool
    {
        return func(name, value);
    });
}

} // namespace rtti
