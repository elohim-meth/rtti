﻿#ifndef METAMETHOD_P_H
#define METAMETHOD_P_H

#include "metaitem_p.h"
#include <rtti/metamethod.h>

namespace rtti {

class RTTI_PRIVATE MetaMethodPrivate: public MetaItemPrivate
{
public:
    MetaMethodPrivate(std::string_view const &name, const MetaContainer &owner,
                      std::unique_ptr<IMethodInvoker> invoker)
        : MetaItemPrivate{name, owner},
          m_invoker{std::move(invoker)}
    {}

private:
    std::unique_ptr<IMethodInvoker> m_invoker;
    friend class rtti::MetaMethod;
};

} // namespace rtti

#endif // METAMETHOD_P_H

