#ifndef METACONSTRUCTOR_H
#define METACONSTRUCTOR_H

#include <rtti/variant.h>

namespace rtti {

struct RTTI_API IConstructorInvoker: IMethodInvoker
{};

class MetaConstructorPrivate;

class RTTI_API MetaConstructor final: public MetaItem
{
    DECLARE_PRIVATE(MetaConstructor)
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
    explicit MetaConstructor(std::string_view name, MetaContainer &owner,
                             std::unique_ptr<IConstructorInvoker> constructor);
    static MetaConstructor* create(std::string_view name, MetaContainer &owner,
                                   std::unique_ptr<IConstructorInvoker> constructor);

private:
    IConstructorInvoker const* constructor() const;

    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    static MetaConstructor* create(std::string_view name, MetaContainer &owner,
                                   std::unique_ptr<IConstructorInvoker> constructor,
                                   CreateAccessKey)
    { return create(name, owner, std::move(constructor)); }

};

}

#endif // METACONSTRUCTOR_H

