#ifndef SFINAE_H
#define SFINAE_H

#include <rtti/function_traits.h>
#include <type_traits>

#define HAS_TYPE(NAME) \
template<typename, typename = void> \
struct has_type_##NAME: std::false_type \
{}; \
template<typename T> \
struct has_type_##NAME<T, std::void_t<typename T::NAME>>: std::true_type \
{};

#define HAS_METHOD(NAME) \
namespace internal { \
template<typename T, bool is_class = std::is_class<T>::value, typename = void> \
struct has_method_##NAME##_helper: std::false_type \
{}; \
\
template<typename T> \
struct has_method_##NAME##_helper<T, true, std::void_t<decltype(&T::NAME)>>: \
    has_method_##NAME##_helper<decltype(&T::NAME)> \
{}; \
\
template<typename Signature> \
struct has_method_##NAME##_helper<Signature, false> \
{ \
\
  static_assert(std::is_member_function_pointer<Signature>::value, "Not a member function pointer"); \
  using T = typename mpl::function_traits<Signature>::class_type; \
  static_assert(!std::is_void<T>::value, "Void class type"); \
\
  template<typename C, typename = std::void_t<decltype(static_cast<Signature>(&C::NAME))>> \
  static auto check(int) -> std::true_type; \
  template<typename> \
  static auto check (...) -> std::false_type; \
\
  using type = decltype(check<T>(0)); \
}; \
}\
template<typename Signature> \
struct has_method_##NAME: internal::has_method_##NAME##_helper<Signature>::type \
{};

#define CAN_CALL_METHOD(NAME) \
namespace internal { \
template<typename T, typename ...Args> \
using result_of_call_method_##NAME = decltype( \
    std::declval<T>().NAME(std::declval<Args>()...)); \
} \
template<typename T, typename Signature, typename = void> \
struct can_call_method_##NAME: std::false_type \
{}; \
template<typename T, typename ...Args> \
struct can_call_method_##NAME<T, void(Args...), \
    std::void_t<internal::result_of_call_method_##NAME<T, Args...>> \
    >: std::true_type \
{}; \
template<typename T, typename R, typename ...Args> \
struct can_call_method_##NAME<T, R(Args...), \
    typename std::enable_if<!std::is_void<R>::value && \
                             std::is_convertible<internal::result_of_call_method_##NAME<T, Args...>, R \
                                                >::value \
                           >::type \
    >: std::true_type \
{};

namespace mpl {
// Non-capturing lambdas have a very interesting property : they can convert to an adequate function pointer,
// but they can also do so implicitly when you apply unary operator + to them

template<typename T, typename = void>
struct lambda_has_capture: std::true_type
{};

template <typename T>
struct lambda_has_capture<T, std::void_t<decltype(+std::declval<T>())>> : std::false_type
{};

} // namespace mpl

#endif // SFINAE_H
