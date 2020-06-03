#ifndef METAMETHOD_IMPL_H
#define METAMETHOD_IMPL_H

namespace rtti {

template<typename ...Args>
variant MetaMethod::invoke(Args&&... args) const
{
    static_assert(sizeof...(Args) <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported metamethod arguments: 10");
    auto interface = invoker();
    if (interface->isStatic())
        return interface->invoke_static(std::forward<Args>(args)...);
    else
        return interface->invoke_method(std::forward<Args>(args)...);
}

} // namespace rtti

#endif // METAMETHOD_IMPL_H
