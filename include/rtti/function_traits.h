#ifndef FUNCTION_TRAITS_H
#define FUNCTION_TRAITS_H

#include <rtti/typelist.h>

#include <functional>
#include <type_traits>

namespace mpl {

namespace internal {

template<typename F>
struct function_traits_helper: function_traits_helper<decltype(&F::operator())>
{};

template<typename R, typename ...Args>
struct function_traits_helper<R(*)(Args...)>
{
    using class_type = void;

    using is_const = std::false_type;
    using is_volatile = std::false_type;
    using is_lrefthis = std::false_type;
    using is_rrefthis = std::false_type;
    using is_noexcept = std::false_type;

    using result_type = R;
    using args = mpl::type_list<Args...>;
    using arity = mpl::typelist_size<args>;
    template<std::size_t i>
    using arg = mpl::typelist_get<args, i>;

    using base_signature = R(Args...);
    using uniform_signature = R(Args...);
};

#if __cplusplus > 201402L
template<typename R, typename ...Args>
struct function_traits_helper<R(*)(Args...) noexcept>:
    function_traits_helper<R(*)(Args...)>
{
    using is_noexcept = std::true_type;
    using base_signature = R(Args...) noexcept;
    using uniform_signature = R(Args...) noexcept;
};
#endif

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...)>:
    function_traits_helper<R(*)(Args...)>
{
    using class_type = C;
    using uniform_signature = R(C&, Args...);
};

#if __cplusplus > 201402L
template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) noexcept>:
    function_traits_helper<R(*)(Args...) noexcept>
{
    using class_type = C;
    using uniform_signature = R(C&, Args...) noexcept;
};
#endif

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) &>:
    function_traits_helper<R(C::*)(Args...)>
{
    using is_lrefthis = std::true_type;
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) &&>:
    function_traits_helper<R(C::*)(Args...)>
{
    using is_rrefthis = std::true_type;
    using uniform_signature = R(C&&, Args...);
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const>:
    function_traits_helper<R(*)(Args...)>
{
    using class_type = C;
    using is_const = std::true_type;
    using uniform_signature = R(C const&, Args...);
};

#if __cplusplus > 201402L
template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const noexcept>:
    function_traits_helper<R(*)(Args...) noexcept>
{
    using class_type = C;
    using is_const = std::true_type;
    using uniform_signature = R(C const&, Args...) noexcept;
};
#endif

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const &>:
    function_traits_helper<R(C::*)(Args...) const>
{
    using is_lrefthis = std::true_type;
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const &&>:
    function_traits_helper<R(C::*)(Args...) const>
{
    using is_rrefthis = std::true_type;
    using uniform_signature = R(C const&&, Args...);
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) volatile>:
    function_traits_helper<R(*)(Args...)>
{
    using class_type = C;
    using is_volatile = std::true_type;
    using uniform_signature = R(volatile C&, Args...);
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) volatile &>:
    function_traits_helper<R(C::*)(Args...) volatile>
{
    using is_lrefthis = std::true_type;
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) volatile &&>:
    function_traits_helper<R(C::*)(Args...) volatile>
{
    using is_rrefthis = std::true_type;
    using uniform_signature = R(volatile C&&, Args...);
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const volatile>:
    function_traits_helper<R(*)(Args...)>
{
    using class_type = C;
    using is_const = std::true_type;
    using is_volatile = std::true_type;
    using uniform_signature = R(C const volatile&, Args...);
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const volatile &>:
    function_traits_helper<R(C::*)(Args...) const volatile>
{
    using is_lrefthis = std::true_type;
};

template<typename C, typename R, typename ...Args>
struct function_traits_helper<R(C::*)(Args...) const volatile &&>:
    function_traits_helper<R(C::*)(Args...) const volatile>
{
    using is_rrefthis = std::true_type;
    using uniform_signature = R(C const volatile&&, Args...);
};

template <typename F>
struct function_traits_helper<std::function<F>>: function_traits_helper<F>
{};

} // namespace internal

template<typename F>
struct function_traits: internal::function_traits_helper<std::decay_t<F>>
{};

// Factory
template<typename F>
using function_type = std::function<typename function_traits<F>::base_signature>;

template<typename F>
inline function_type<F> make_function(F &&f)
{
    return function_type<F>{std::forward<F>(f)};
}

} // namespace mpl

#endif // FUNCTION_TRAITS_H

