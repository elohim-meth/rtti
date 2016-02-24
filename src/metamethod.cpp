#include "metamethod_p.h"
#include "metacontainer_p.h"

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaMethod
//--------------------------------------------------------------------------------------------------------------------------------

const IMethodInvoker* MetaMethod::invoker() const
{
    auto d = d_func();
    return d->m_invoker.get();
}

MetaCategory MetaMethod::category() const
{
    return mcatMethod;
}

MetaMethod::MetaMethod(std::string &&name, MetaContainer &owner,
                       std::unique_ptr<IMethodInvoker> invoker)
    : MetaItem(*new MetaMethodPrivate{std::move(name), owner, std::move(invoker)})
{}

MetaMethod* MetaMethod::create(const char *name, MetaContainer &owner,
                               std::unique_ptr<IMethodInvoker> invoker)
{
    if (!invoker)
        return nullptr;

    auto signature = invoker->signature(name);
    auto result = const_cast<MetaMethod*>(owner.getMethod(signature.c_str()));
    if (!result)
    {
        result = new MetaMethod{std::move(signature), owner, std::move(invoker)};
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

} // namespace rtti
