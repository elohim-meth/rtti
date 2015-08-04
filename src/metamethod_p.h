#ifndef METAMETHOD_P_H
#define METAMETHOD_P_H

#include "metaitem_p.h"
#include "metamethod.h"

namespace rtti {

class DLL_LOCAL MetaMethodPrivate: public MetaItemPrivate
{
public:
    MetaMethodPrivate(std::string &&name, const MetaContainer &owner,
                      std::unique_ptr<IMethodInvoker> invoker)
        : MetaItemPrivate{std::move(name), owner},
          m_invoker{std::move(invoker)}
    {}

private:
    std::unique_ptr<IMethodInvoker> m_invoker;
    friend class rtti::MetaMethod;
};

} // namespace rtti

#endif // METAMETHOD_P_H

