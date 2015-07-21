#ifndef METACONSTRUCTOR_P_H
#define METACONSTRUCTOR_P_H

#include "metacontainer_p.h"
#include "metaconstructor.h"

namespace rtti {

class DLL_LOCAL MetaConstructorPrivate: public MetaItemPrivate
{
public:
    MetaConstructorPrivate(std::string &&name, const MetaContainer &owner,
                           std::unique_ptr<IConstructorInvoker> constructor)
        : MetaItemPrivate{std::move(name), owner},
          m_constructor{std::move(constructor)}
    {}

private:
    std::unique_ptr<IConstructorInvoker> m_constructor;
    friend class rtti::MetaConstructor;
};

} // namespace rtti

#endif // METACONSTRUCTOR_P_H

