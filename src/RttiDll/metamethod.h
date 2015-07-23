#ifndef METHOD_H
#define METHOD_H

#include "metaitem.h"
#include "argument.h"

namespace rtti {

struct DLL_PUBLIC IMethodInvoker
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
    virtual ~IMethodInvoker() noexcept = default;
};

class MetaMethodPrivate;

class DLL_PUBLIC MetaMethod final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
private:
    DECLARE_PRIVATE(MetaMethod)
};

} // namespace rtti

#endif // METHOD_H

