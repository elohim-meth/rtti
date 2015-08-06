﻿#include "metaconstructor_p.h"

namespace rtti {

MetaConstructor::MetaConstructor(std::string &&name, MetaContainer &owner,
                                 std::unique_ptr<IConstructorInvoker> constructor)
    : MetaItem{*new MetaConstructorPrivate{std::move(name), owner, std::move(constructor)}}
{}

MetaConstructor* MetaConstructor::create(const char *name, MetaContainer &owner,
                                         std::unique_ptr<IConstructorInvoker> constructor)
{
    if (!constructor)
        return nullptr;

    auto temp = name ? std::string{name} : constructor->signature();
    auto result = const_cast<MetaConstructor*>(owner.getConstructor(temp.c_str()));
    if (!result)
    {
        result = new MetaConstructor(std::move(temp), owner, std::move(constructor));
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

const IConstructorInvoker* MetaConstructor::constructor() const
{
    auto d = d_func();
    return d->m_constructor.get();
}

MetaCategory MetaConstructor::category() const
{
    return mcatConstructor;
}

} // namespace rtti
