#ifndef MISC_TRAITS_H
#define MISC_TRAITS_H

#include <typelist.h>
#include <sfinae.h>

namespace rtti {

//-----------------------------------------------------------------------------------------------------------------------------
// Until C++14

template<typename T>
using add_const_t = typename std::add_const<T>::type;

template<typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template<typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template<typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template<bool C, typename T, typename F>
using conditional_t = typename std::conditional<C, T, F>::type;

template<typename T>
using decay_t = typename std::decay<T>::type;

template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template<typename T>
using is_array_t = typename std::is_array<T>::type;

template<typename T>
using is_class_t = typename std::is_class<T>::type;

template<typename T>
using is_const_t = typename std::is_const<T>::type;

template<typename T>
using is_reference_t = typename std::is_reference<T>::type;

template<typename T>
using is_lvalue_reference_t = typename std::is_lvalue_reference<T>::type;

template<typename T>
using is_rvalue_reference_t = typename std::is_rvalue_reference<T>::type;

template<typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template<typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template<typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template<typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template<typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template<typename T>
using is_default_constructible_t = typename std::is_default_constructible<T>::type;

#if __GNUC__ < 5
template<typename T>
using is_trivially_default_constructible_t = typename std::has_trivial_default_constructor<T>::type;
#else
template<typename T>
using is_trivially_default_constructible_t = typename std::is_trivially_default_constructible<T>::type;
#endif

template<typename T>
using is_move_constructible_t = typename std::is_move_constructible<T>::type;

template<typename T>
using is_copy_constructible_t = typename std::is_copy_constructible<T>::type;

template<typename T>
using is_trivially_destructible_t = typename std::is_trivially_destructible<T>::type;

template<typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, std::size_t I>
struct add_pointers: add_pointers<add_pointer_t<T>, I - 1>
{};

template<typename T>
struct add_pointers<T, 0>: mpl::identity<T>
{};

template<typename T, std::size_t I>
using add_pointers_t = typename add_pointers<T, I>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_pointer<T>::value>
struct pointer_arity;

template<typename T>
struct pointer_arity<T, false>:
    std::integral_constant<std::size_t, 0>
{};

template<typename T>
struct pointer_arity<T, true>:
    std::integral_constant<std::size_t, pointer_arity<remove_pointer_t<T>>::value + 1>
{};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_pointer<T>::value>
struct remove_all_pointers;

template<typename T>
struct remove_all_pointers<T, false>: mpl::identity<remove_cv_t<T>>
{};

template<typename T>
struct remove_all_pointers<T, true>: remove_all_pointers<remove_pointer_t<T>>
{};

template<typename T>
using remove_all_pointers_t = typename remove_all_pointers<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------


template<typename T>
struct array_length: std::integral_constant<std::size_t, 1>
{};

template<typename T, std::size_t N>
struct array_length<T[N]>: std::integral_constant<std::size_t, N * array_length<T>::value>
{};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_reference<T>::value,
                     bool = std::is_pointer<T>::value,
                     bool = std::is_array<T>::value>
struct remove_all_cv;

template<typename T>
struct remove_all_cv<T, false, false, false>
{
    using type = remove_cv_t<T>;
};

template<typename T>
struct remove_all_cv<T, true, false, false>
{
    constexpr static bool lref = is_lvalue_reference_t<T>::value;
    using U = typename remove_all_cv<remove_reference_t<T>>::type;
    using type = conditional_t<lref, add_lvalue_reference_t<U>, add_rvalue_reference_t<U>>;
};

template<typename T>
struct remove_all_cv<T, false, true, false>
{
    using U = typename remove_all_cv<remove_pointer_t<T>>::type;
    using type = add_pointer_t<U>;
};

template<typename T>
struct remove_all_cv<T, false, false, true>
{
    constexpr static std::size_t extent = std::extent<T>::value;
    using U = typename remove_all_cv<remove_extent_t<T>>::type;
    using type = U[extent];
};

template<typename T>
using remove_all_cv_t = typename remove_all_cv<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
using full_decay_t = remove_all_cv_t<decay_t<T>>;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_reference<T>::value,
                     bool = std::is_pointer<T>::value,
                     bool = std::is_array<T>::value>
struct base_type;

template<typename T>
struct base_type<T, false, false, false>: mpl::identity<remove_cv_t<T>>
{};

template<typename T>
struct base_type<T, true, false, false>: base_type<remove_reference_t<T>>
{};

template<typename T>
struct base_type<T, false, true, false>: base_type<remove_pointer_t<T>>
{};

template<typename T>
struct base_type<T, false, false, true>: base_type<remove_all_extents_t<T>>
{};

template<typename T>
using base_type_t = typename base_type<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

namespace internal {

template<typename T,
         std::size_t I,
         bool = std::is_const<T>::value,
         bool = std::is_pointer<T>::value>
struct const_bitset_helper;

template<typename T, std::size_t I>
struct const_bitset_helper<T, I, false, false>:
    std::integral_constant<std::size_t, 0>
{};

template<typename T, std::size_t I>
struct const_bitset_helper<T, I, true, false>:
    std::integral_constant<std::size_t, 1 << I>
{};

template<typename T, std::size_t I>
struct const_bitset_helper<T, I, false, true>:
    std::integral_constant<std::size_t, const_bitset_helper<remove_pointer_t<T>, I - 1>::value>
{};

template<typename T, std::size_t I>
struct const_bitset_helper<T, I, true, true>:
    std::integral_constant<std::size_t, (1 << I) + const_bitset_helper<remove_pointer_t<T>, I - 1>::value>
{};

} // namespace internal

template<typename T>
using const_bitset = internal::const_bitset_helper<T, pointer_arity<T>::value>;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, typename ...Args>
struct is_converting_constructor: std::false_type
{};

template<typename T>
struct is_converting_constructor<T>: std::false_type
{};

template<typename T, typename Arg>
struct is_converting_constructor<T, Arg>:
    std::conditional<!std::is_same<T, full_decay_t<Arg>>::value &&
                      std::is_convertible<Arg, T>::value,
                     std::true_type, std::false_type>::type
{};

template<typename T, typename ...Args>
using is_converting_constructor_t = typename is_converting_constructor<T, Args...>::type;


template<typename T>
struct is_lvalue_const_reference:
    conditional_t<is_lvalue_reference_t<T>::value && is_const_t<remove_reference_t<T>>::value, std::true_type, std::false_type>
{};

template<typename T>
using is_lvalue_const_reference_t = typename is_lvalue_const_reference<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
using is_class_ptr =
conditional_t<
    std::is_pointer<remove_reference_t<T>>::value &&
    std::is_class<remove_reference_t<remove_pointer_t<T>>>::value,
    std::true_type, std::false_type
>;

template<typename T>
using is_class_ptr_t = typename is_class_ptr<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename E>
struct enable_bitmask_enum: std::false_type
{};

template<typename E,
         typename = enable_if_t<enable_bitmask_enum<E>::value>>
constexpr E operator|(E lhs, E rhs)
{
  using T = typename std::underlying_type<E>::type;
  return static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

template<typename E,
         typename = enable_if_t<enable_bitmask_enum<E>::value>>
constexpr E operator&(E lhs, E rhs)
{
  using T = typename std::underlying_type<E>::type;
  return static_cast<E>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

#define BITMASK_ENUM(NAME) \
template<> \
struct enable_bitmask_enum<NAME>: std::true_type \
{} \

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_class<T>::value>
struct has_move_constructor;

template<typename T>
struct has_move_constructor<T, false>: std::is_move_constructible<T>
{};

template<typename T>
struct only_copy_available
{
    template<typename U>
    struct convert
    {
        convert() = default;
        operator U&& ();
        operator const U& ();
    };

    template<typename U>
    static auto check(decltype(new U(convert<U>{}))) -> std::true_type;

    template<typename U>
    static auto check (const volatile U*) -> std::false_type;

    using type = decltype(check<T>((T*)nullptr));
    static constexpr bool value = type::value;
};

template<typename T>
struct has_move_constructor<T, true>:
    conditional_t<
#if __GNUC__ >= 5
        std::is_trivially_move_constructible<T>::value ||
#endif
       (std::is_move_constructible<T>::value && !only_copy_available<T>::value),
    std::true_type, std::false_type>
{};

template<typename T>
using has_move_constructor_t = typename has_move_constructor<T>::type;

} // namespace rtti

#endif // MISC_TRAITS_H

