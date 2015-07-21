#include"metanamespace_p.h"

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaNamespace
//--------------------------------------------------------------------------------------------------------------------------------

MetaNamespace::MetaNamespace()
    : MetaContainer(*new MetaNamespacePrivate{"global"})
{}

const MetaNamespace* MetaNamespace::global() noexcept
{
    static MetaNamespace globalNamespace;
    return &globalNamespace;
}

bool MetaNamespace::isGlobal() const noexcept
{
    auto d = d_func();
    return (d->owner() == nullptr);
}

MetaNamespace* MetaNamespace::create(const char *name, MetaContainer &owner)
{
    auto result = const_cast<MetaNamespace*>(owner.getNamespace(name));
    if (!result)
    {
        result = new MetaNamespace(name, owner);
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

MetaCategory MetaNamespace::category() const noexcept
{
    return mcatNamespace;
}

} // namespace rtti
