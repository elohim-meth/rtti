#ifndef MISC_TRAITS_H
#define MISC_TRAITS_H

#include <rtti/typelist.h>

#include <string>

namespace rtti {

// It's weird STD library lacks this operator :()
template<typename _CharT, typename _Traits, typename _Alloc>
inline std::basic_string<_CharT, _Traits, _Alloc>
operator+(std::basic_string<_CharT, _Traits, _Alloc> const &lhs,
          std::basic_string_view<_CharT, _Traits> rhs)
{
    std::basic_string<_CharT, _Traits, _Alloc> str(lhs);
    str.append(rhs);
    return str;
}

template<typename _CharT, typename _Traits, typename _Alloc>
inline std::basic_string<_CharT, _Traits, _Alloc>
operator+(std::basic_string<_CharT, _Traits, _Alloc> &&lhs,
          std::basic_string_view<_CharT, _Traits> rhs)
{
    return std::move(lhs.append(rhs));
}

template<bool B, typename T, typename F>
constexpr auto conditional_v = std::conditional_t<B, T, F>::value;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
struct size_of: std::integral_constant<std::size_t, sizeof(T)>
{};

template<>
struct size_of<void>: std::integral_constant<std::size_t, 0>
{};

template<typename T>
constexpr auto size_of_v = size_of<T>::value;

template<typename T>
using size_of_t = typename size_of<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, std::size_t I>
struct add_pointers: add_pointers<std::add_pointer_t<T>, I - 1>
{};

template<typename T>
struct add_pointers<T, 0>: mpl::identity<T>
{};

template<typename T, std::size_t I>
using add_pointers_t = typename add_pointers<T, I>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_pointer_v<T>>
struct pointer_arity;

template<typename T>
constexpr auto pointer_arity_v = pointer_arity<T>::value;

template<typename T>
struct pointer_arity<T, false>:
    std::integral_constant<std::size_t, 0>
{};

template<typename T>
struct pointer_arity<T, true>:
    std::integral_constant<std::size_t, pointer_arity_v<std::remove_pointer_t<T>> + 1>
{};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_pointer_v<T>>
struct remove_all_pointers;

template<typename T>
struct remove_all_pointers<T, false>: mpl::identity<std::remove_cv_t<T>>
{};

template<typename T>
struct remove_all_pointers<T, true>: remove_all_pointers<std::remove_pointer_t<T>>
{};

template<typename T>
using remove_all_pointers_t = typename remove_all_pointers<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------


template<typename T>
struct array_length: std::integral_constant<std::size_t, 1>
{};

template<typename T>
constexpr auto array_length_v = array_length<T>::value;

template<typename T, std::size_t N>
struct array_length<T[N]>: std::integral_constant<std::size_t, N * array_length_v<T>>
{};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_reference_v<T>,
                     bool = std::is_pointer_v<T>,
                     bool = std::is_array_v<T>>
struct remove_all_cv;

template<typename T>
using remove_all_cv_t = typename remove_all_cv<T>::type;

template<typename T>
struct remove_all_cv<T, false, false, false>
{
    using type = std::remove_cv_t<T>;
};

template<typename T>
struct remove_all_cv<T, true, false, false>
{
    constexpr static auto lref = std::is_lvalue_reference_v<T>;
    using U = remove_all_cv_t<std::remove_reference_t<T>>;
    using type = std::conditional_t<lref, std::add_lvalue_reference_t<U>, std::add_rvalue_reference_t<U>>;
};

template<typename T>
struct remove_all_cv<T, false, true, false>
{
    using U = typename remove_all_cv<std::remove_pointer_t<T>>::type;
    using type = std::add_pointer_t<U>;
};

template<typename T>
struct remove_all_cv<T, false, false, true>
{
    constexpr static auto extent = std::extent_v<T>;
    using U = remove_all_cv_t<std::remove_extent_t<T>>;
    using type = U[extent];
};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
using full_decay_t = remove_all_cv_t<std::decay_t<T>>;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_reference_v<T>,
                     bool = std::is_pointer_v<T>,
                     bool = std::is_array_v<T>>
struct base_type;

template<typename T>
struct base_type<T, false, false, false>: mpl::identity<std::remove_cv_t<T>>
{};

template<typename T>
struct base_type<T, true, false, false>: base_type<std::remove_reference_t<T>>
{};

template<typename T>
struct base_type<T, false, true, false>: base_type<std::remove_pointer_t<T>>
{};

template<typename T>
struct base_type<T, false, false, true>: base_type<std::remove_all_extents_t<T>>
{};

template<typename T>
using base_type_t = typename base_type<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

namespace internal {

template<typename T,
         std::size_t I,
         bool = std::is_const_v<T>,
         bool = std::is_pointer_v<T>>
struct const_bitset_helper;

template<typename T, std::size_t I>
constexpr auto const_bitset_helper_v = const_bitset_helper<T, I>::value;

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
    std::integral_constant<std::size_t, const_bitset_helper_v<std::remove_pointer_t<T>, I - 1>>
{};

template<typename T, std::size_t I>
struct const_bitset_helper<T, I, true, true>:
    std::integral_constant<std::size_t, (1 << I) + const_bitset_helper_v<std::remove_pointer_t<T>, I - 1>>
{};

} // namespace internal

template<typename T>
using const_bitset = internal::const_bitset_helper<T, pointer_arity<T>::value>;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, typename ...Args>
struct is_converting_constructor: std::false_type
{};

template<typename T, typename Arg>
struct is_converting_constructor<T, Arg>:
    std::conditional_t<!std::is_same_v<T, full_decay_t<Arg>> && std::is_convertible_v<Arg, T>,
                       std::true_type, std::false_type>
{};

template<typename T, typename ...Args>
using is_converting_constructor_t = typename is_converting_constructor<T, Args...>::type;

template<typename T, typename ...Args>
constexpr auto is_converting_constructor_v = is_converting_constructor<T, Args...>::value;

template<typename T>
struct is_lvalue_const_reference:
    std::conditional_t<std::is_lvalue_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>,
                       std::true_type, std::false_type>
{};

template<typename T>
using is_lvalue_const_reference_t = typename is_lvalue_const_reference<T>::type;

template<typename T>
constexpr auto is_lvalue_const_reference_v = is_lvalue_const_reference<T>::value;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
using is_class_ptr =
std::conditional_t<
    std::is_pointer_v<std::remove_reference_t<T>> &&
    std::is_class_v<std::remove_pointer_t<std::remove_reference_t<T>>>,
    std::true_type, std::false_type
>;

template<typename T>
using is_class_ptr_t = typename is_class_ptr<T>::type;

template<typename T>
constexpr auto is_class_ptr_v = is_class_ptr_t<T>::value;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename E>
struct enable_bitmask_enum: std::false_type
{};

template<typename E>
constexpr auto enable_bitmask_enum_v = enable_bitmask_enum<E>::value;

template<typename E,
         typename = std::enable_if_t<std::is_enum_v<E> & enable_bitmask_enum_v<E>>>
constexpr E operator|(E lhs, E rhs)
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

template<typename E,
         typename = std::enable_if_t<std::is_enum_v<E> & enable_bitmask_enum_v<E>>>
constexpr E operator&(E lhs, E rhs)
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

#define BITMASK_ENUM(NAME) \
template<> \
struct enable_bitmask_enum<NAME>: std::true_type \
{};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = std::is_class_v<T>>
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
    static constexpr auto value = type::value;
};

template<typename T>
struct has_move_constructor<T, true>:
    std::conditional_t<
#if __GNUC__ >= 5
        std::is_trivially_move_constructible<T>::value ||
#endif
       (std::is_move_constructible<T>::value && !only_copy_available<T>::value),
    std::true_type, std::false_type>
{};

template<typename T>
using has_move_constructor_t = typename has_move_constructor<T>::type;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, typename U, typename = std::void_t<>>
struct has_eq: std::false_type
{};

template<typename T, typename U, typename = std::void_t<>>
struct has_nt_eq: std::false_type
{};

template<typename T, typename U>
struct has_eq<T, U, std::void_t<decltype(std::declval<T>() == std::declval<U>())>>
    : std::true_type
{};

template<typename T, typename U>
struct has_nt_eq<T, U, std::void_t<decltype(std::declval<T>() == std::declval<U>())>>
    : std::bool_constant<noexcept(std::declval<T>() == std::declval<U>())>
{};

template<typename T, typename U>
using has_eq_t = typename has_eq<T, U>::type;

template<typename T, typename U>
constexpr auto has_eq_v = has_eq<T, U>::value;

template<typename T, typename U>
using has_nt_eq_t = typename has_nt_eq<T, U>::type;

template<typename T, typename U>
constexpr auto has_nt_eq_v = has_nt_eq<T, U>::value;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
struct is_template_type: std::false_type
{
    using args = mpl::empty_list;
};

template<template <typename... > class T, typename ...Args>
struct is_template_type<T<Args...>>: std::true_type
{
    using args = mpl::type_list<Args...>;
};

template<template <auto... > class T, auto ...Vals>
struct is_template_type<T<Vals...>>: std::true_type
{
    using args = mpl::type_list<decltype(Vals)...>;
};

template<template <typename, auto> class T, typename Arg, auto Val>
struct is_template_type<T<Arg, Val>>: std::true_type
{
    using args = mpl::type_list<Arg, decltype(Val)>;
};

template<template <auto, typename> class T, auto Val, typename Arg>
struct is_template_type<T<Val, Arg>>: std::true_type
{
    using args = mpl::type_list<decltype(Val), Arg>;
};

template<template <auto, typename, typename> class T, auto Val, typename Arg1, typename Arg2>
struct is_template_type<T<Val, Arg1, Arg2>>: std::true_type
{
    using args = mpl::type_list<decltype(Val), Arg1, Arg2>;
};

template<template <typename, typename, auto> class T, typename Arg1, typename Arg2, auto Val>
struct is_template_type<T<Arg1, Arg2, Val>>: std::true_type
{
    using args = mpl::type_list<Arg1, Arg2, decltype(Val)>;
};

template<template <auto, typename, typename, typename> class T, auto Val, typename Arg1, typename Arg2, typename Arg3>
struct is_template_type<T<Val, Arg1, Arg2, Arg3>>: std::true_type
{
    using args = mpl::type_list<decltype(Val), Arg1, Arg2, Arg3>;
};

template<template <typename, typename, typename, auto> class T, typename Arg1, typename Arg2, typename Arg3, auto Val>
struct is_template_type<T<Arg1, Arg2, Arg3, Val>>: std::true_type
{
    using args = mpl::type_list<Arg1, Arg2, Arg3, decltype(Val)>;
};

template<typename T>
using is_template_type_t = typename is_template_type<T>::type;

template<typename T>
using template_type_args_t = typename is_template_type<T>::args;

template<typename T, typename U>
constexpr auto is_template_type_v = is_template_type<T>::value;


} // namespace rtti

#endif // MISC_TRAITS_H

