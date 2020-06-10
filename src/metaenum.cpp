#include "metaenum_p.h"
#include "metacontainer_p.h"

namespace rtti {

MetaEnum::MetaEnum(std::string_view name, MetaContainer const &owner, MetaType_ID typeId)
    : MetaItem{*new MetaEnumPrivate{name, owner, typeId}}
{}

MetaEnum* MetaEnum::create(std::string_view name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaEnum *>(owner.getEnum(name));
    if (!result)
    {
        result = new MetaEnum{name, owner, typeId};
        if (!INVOKE_PROTECTED(owner, addItem, result))
        {
            delete result;
            result = const_cast<MetaEnum *>(owner.getEnum(name));
        }
    }
    return result;
}

void MetaEnum::addElement(std::string_view name, variant &&value)
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

variant const& MetaEnum::element(std::size_t index) const
{
    auto d = d_func();
    return d->m_elements.get(index);
}

std::string const& MetaEnum::elementName(std::size_t index) const
{
    auto d = d_func();
    return d->m_elements.name(index);
}

variant const& MetaEnum::element(std::string_view name) const
{
    auto d = d_func();
    return d->m_elements.get(name);
}

void MetaEnum::for_each_element(enum_element_t const &func) const
{
    if (!func)
        return;

    auto d = d_func();
    d->m_elements.for_each([&func](std::string const &name, variant const &value) -> auto
    {
        return func(name, value);
    });
}

} // namespace rtti
