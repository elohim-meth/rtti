#include"metanamespace_p.h"

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaNamespace
//--------------------------------------------------------------------------------------------------------------------------------

MetaNamespace::MetaNamespace()
    : MetaContainer{*new MetaNamespacePrivate{"global"}}
{}

MetaNamespace const* MetaNamespace::global()
{
    static MetaNamespace globalNamespace;
    return &globalNamespace;
}

bool MetaNamespace::isGlobal() const
{
    auto d = d_func();
    return (d->owner() == nullptr);
}

MetaNamespace* MetaNamespace::create(std::string_view const &name, MetaContainer &owner)
{
    auto result = const_cast<MetaNamespace*>(owner.getNamespace(name));
    if (!result)
    {
        result = new MetaNamespace{name, owner};
        INVOKE_PROTECTED(owner, addItem, result);
    }
    return result;
}

MetaCategory MetaNamespace::category() const
{
    return mcatNamespace;
}

} // namespace rtti
