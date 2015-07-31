#ifndef METAPROPERTY_P_H
#define METAPROPERTY_P_H

#include "metaproperty.h"
#include "metaitem_p.h"

namespace rtti {

class DLL_LOCAL MetaPropertyPrivate: public MetaItemPrivate
{
public:
    MetaPropertyPrivate(const char *name, const MetaContainer &owner,
                      std::unique_ptr<IPropertyInvoker> invoker)
        : MetaItemPrivate{name, owner},
          m_invoker{std::move(invoker)}
    {}

private:
    std::unique_ptr<IPropertyInvoker> m_invoker;
    friend class rtti::MetaProperty;
};

} // namespace rtti

#endif // METAPROPERTY_P_H

