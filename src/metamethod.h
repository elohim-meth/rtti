#ifndef METHOD_H
#define METHOD_H

#include "metaitem.h"
#include "variant.h"
#include "argument.h"

#include <vector>
#include <memory>

namespace rtti {

struct DLL_PUBLIC IMethodInvoker
{
    enum {
        MaxNumberOfArguments = 10
    };

    virtual bool isStatic() const = 0;
    virtual MetaType_ID returnTypeId() const = 0;
    virtual std::vector<MetaType_ID> parametersTypeId() const = 0;
    virtual std::string signature(const char *name = nullptr) const = 0;
    virtual variant invoke_static(argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual variant invoke_method(const variant &instance,
                                  argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual variant invoke_method(variant &instance,
                                  argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual ~IMethodInvoker() = default;
};

class MetaMethodPrivate;

class DLL_PUBLIC MetaMethod final: public MetaItem
{
public:
    MetaCategory category() const override;

    template<typename ...Args>
    variant invoke(Args&&... args) const
    {
        static_assert(sizeof...(Args) <= IMethodInvoker::MaxNumberOfArguments,
                      "Maximum supported metamethod arguments: 10");
        auto interface = invoker();
        if (interface->isStatic())
            return interface->invoke_static(std::forward<Args>(args)...);
        else
            return interface->invoke_method(std::forward<Args>(args)...);
    }
protected:
    explicit MetaMethod(std::string &&name, MetaContainer &owner,
                        std::unique_ptr<IMethodInvoker> invoker);
    static MetaMethod* create(const char *name, MetaContainer &owner,
                              std::unique_ptr<IMethodInvoker> invoker);
private:
    const IMethodInvoker* invoker() const;

    DECLARE_PRIVATE(MetaMethod)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METHOD_H

