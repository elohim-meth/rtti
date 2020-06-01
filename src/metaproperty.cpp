#include "metaproperty_p.h"
#include "metacontainer_p.h"

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaProperty
//--------------------------------------------------------------------------------------------------------------------------------

MetaProperty::MetaProperty(std::string_view name, MetaContainer &owner,
                           std::unique_ptr<IPropertyInvoker> invoker)
    : MetaItem{*new MetaPropertyPrivate{name, owner, std::move(invoker)}}
{}

MetaProperty* MetaProperty::create(std::string_view name, MetaContainer &owner,
                                   std::unique_ptr<IPropertyInvoker> invoker)
{
    if (name.empty() || !invoker)
        return nullptr;

    auto result = const_cast<MetaProperty*>(owner.getProperty(name));
    if (!result)
    {
        result = new MetaProperty{name, owner, std::move(invoker)};
        if (!INVOKE_PROTECTED(owner, addItem, result))
        {
            delete result;
            result = const_cast<MetaProperty*>(owner.getProperty(name));
        }
    }
    return result;
}

IPropertyInvoker const* MetaProperty::invoker() const
{
    auto d = d_func();
    return d->m_invoker.get();
}

MetaCategory MetaProperty::category() const
{
    return mcatProperty;
}

} // namespace rtti
