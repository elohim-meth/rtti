#include "metaconstructor_p.h"

namespace rtti {

MetaConstructor::MetaConstructor(std::string_view name, MetaContainer &owner,
                                 std::unique_ptr<IConstructorInvoker> constructor)
    : MetaItem{*new MetaConstructorPrivate{name, owner, std::move(constructor)}}
{}

MetaConstructor* MetaConstructor::create(std::string_view name, MetaContainer &owner,
                                         std::unique_ptr<IConstructorInvoker> constructor)
{
    if (!constructor)
        return nullptr;

    auto signature = constructor->signature(name.empty() ? CONSTRUCTOR_SIG : name);
    auto result = const_cast<MetaConstructor*>(owner.getConstructor(signature));
    if (!result)
    {
        result = new MetaConstructor{signature, owner, std::move(constructor)};
        if (!INVOKE_PROTECTED(owner, addItem, result))
        {
            delete result;
            result = const_cast<MetaConstructor*>(owner.getConstructor(signature));
        }
    }
    return result;
}

IConstructorInvoker const* MetaConstructor::constructor() const
{
    auto d = d_func();
    return d->m_constructor.get();
}

MetaCategory MetaConstructor::category() const
{
    return mcatConstructor;
}

} // namespace rtti
