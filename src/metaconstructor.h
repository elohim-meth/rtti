#ifndef METACONSTRUCTOR_H
#define METACONSTRUCTOR_H

#include "metamethod.h"

namespace rtti {

struct DLL_PUBLIC IConstructorInvoker: IMethodInvoker
{};

class MetaConstructorPrivate;

class DLL_PUBLIC MetaConstructor final: public MetaItem
{
public:
    MetaCategory category() const override;

    template<typename ...Args>
    variant invoke(Args&&... args) const
    {
        static_assert(sizeof...(Args) <= IConstructorInvoker::MaxNumberOfArguments,
                      "Maximum supported metaconstructor arguments: 10");
        return constructor()->invoke_static(std::forward<Args>(args)...);
    }

protected:
    explicit MetaConstructor(std::string &&name, MetaContainer &owner,
                             std::unique_ptr<IConstructorInvoker> constructor);
    static MetaConstructor* create(char const *name, MetaContainer &owner,
                                   std::unique_ptr<IConstructorInvoker> constructor);

private:
    IConstructorInvoker const* constructor() const;

    DECLARE_PRIVATE(MetaConstructor)
    template<typename, typename> friend class rtti::meta_define;
};

}

#endif // METACONSTRUCTOR_H

