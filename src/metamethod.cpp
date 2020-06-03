#include "metamethod_p.h"
#include "metacontainer_p.h"

namespace rtti {

IMethodInvoker const* MetaMethod::invoker() const
{
    auto d = d_func();
    return d->m_invoker.get();
}

MetaCategory MetaMethod::category() const
{
    return mcatMethod;
}

MetaMethod::MetaMethod(std::string_view name, MetaContainer &owner,
                       std::unique_ptr<IMethodInvoker> invoker)
    : MetaItem(*new MetaMethodPrivate{name, owner, std::move(invoker)})
{}

MetaMethod* MetaMethod::create(std::string_view name, MetaContainer &owner,
                               std::unique_ptr<IMethodInvoker> invoker)
{
    if (!invoker)
        return nullptr;

    auto signature = invoker->signature(name);
    auto result = const_cast<MetaMethod*>(owner.getMethod(signature));
    if (!result)
    {
        result = new MetaMethod{signature, owner, std::move(invoker)};
        if (!INVOKE_PROTECTED(owner, addItem, result))
        {
            delete result;
            result = const_cast<MetaMethod*>(owner.getMethod(signature));
        }
    }
    return result;
}

} // namespace rtti
