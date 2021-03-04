#ifndef METAPROPERTY_P_H
#define METAPROPERTY_P_H

#include "metaitem_p.h"
#include <rtti/variant.h>

namespace rtti {

class RTTI_PRIVATE MetaPropertyPrivate: public MetaItemPrivate
{
public:
    MetaPropertyPrivate(std::string_view name, MetaContainer const &owner,
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

