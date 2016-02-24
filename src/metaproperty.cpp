#include "metaproperty_p.h"
#include "metacontainer_p.h"

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaProperty
//--------------------------------------------------------------------------------------------------------------------------------

const IPropertyInvoker* MetaProperty::invoker() const
{
    auto d = d_func();
    return d->m_invoker.get();
}

MetaCategory MetaProperty::category() const
{
    return mcatProperty;
}

MetaProperty::MetaProperty(const char *name, MetaContainer &owner,
                           std::unique_ptr<IPropertyInvoker> invoker)
    : MetaItem{*new MetaPropertyPrivate{name, owner, std::move(invoker)}}
{}

MetaProperty* MetaProperty::create(const char *name, MetaContainer &owner,
                                   std::unique_ptr<IPropertyInvoker> invoker)
{
    if (!name || !invoker)
        return nullptr;

    auto result = const_cast<MetaProperty*>(owner.getProperty(name));
    if (!result)
    {
        result = new MetaProperty{name, owner, std::move(invoker)};
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

} // namespace rtti
