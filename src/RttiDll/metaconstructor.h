#ifndef METACONSTRUCTOR_H
#define METACONSTRUCTOR_H

#include "metaitem.h"
#include "variant.h"
#include "argument.h"

#include <memory>

namespace rtti {

struct DLL_PUBLIC IConstructorInvoker
{
    enum {
        MaxNumberOfArguments = 10
    };

    virtual variant invoke(argument arg0 = argument{},
                           argument arg1 = argument{},
                           argument arg2 = argument{},
                           argument arg3 = argument{},
                           argument arg4 = argument{},
                           argument arg5 = argument{},
                           argument arg6 = argument{},
                           argument arg7 = argument{},
                           argument arg8 = argument{},
                           argument arg9 = argument{}) const = 0;
    virtual std::string signature() const = 0;
    virtual ~IConstructorInvoker() noexcept = default;
};

class MetaConstructorPrivate;

class DLL_PUBLIC MetaConstructor final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
    template<typename ...Args>
    variant invoke(Args&&... args) const
    {
        static_assert(sizeof...(Args) <= IConstructorInvoker::MaxNumberOfArguments,
                      "Maximum supported metaconstructor arguments: 10");
        return constructor()->invoke(std::forward<Args>(args)...);
    }

protected:
    explicit MetaConstructor(std::string &&name, MetaContainer &owner,
                             std::unique_ptr<IConstructorInvoker> constructor);
    static MetaConstructor* create(const char *name, MetaContainer &owner,
                                   std::unique_ptr<IConstructorInvoker> constructor);

private:
    const IConstructorInvoker* constructor() const noexcept;

    DECLARE_PRIVATE(MetaConstructor)
    template<typename, typename> friend class rtti::meta_define;
};

}

#endif // METACONSTRUCTOR_H

