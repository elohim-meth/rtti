#ifndef METACONSTRUCTOR_P_H
#define METACONSTRUCTOR_P_H

#include "metacontainer_p.h"

#include <rtti/metaconstructor.h>

namespace rtti {

class RTTI_PRIVATE MetaConstructorPrivate: public MetaItemPrivate
{
public:
    MetaConstructorPrivate(std::string_view name, MetaContainer const &owner,
                           std::unique_ptr<IConstructorInvoker> constructor)
        : MetaItemPrivate{name, owner}
        , m_constructor{std::move(constructor)}
    {}

private:
    std::unique_ptr<IConstructorInvoker> m_constructor;
    friend class rtti::MetaConstructor;
};

} // namespace rtti

#endif // METACONSTRUCTOR_P_H
