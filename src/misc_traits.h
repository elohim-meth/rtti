#ifndef MISC_TRAITS_H
#define MISC_TRAITS_H

#include <typelist.h>
#include <sfinae.h>

namespace rtti {

//-----------------------------------------------------------------------------------------------------------------------------
// Until C++14
//-----------------------------------------------------------------------------------------------------------------------------
#if __cplusplus == 201103L

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
using underlying_type_t = typename std::underlying_type<T>::type;

#endif

template<typename T>
constexpr auto is_void_v = std::is_void<T>::value;

template<typename T>
using is_array_t = typename std::is_array<T>::type;

template<typename T>
constexpr auto is_array_v = std::is_array<T>::value;

template<typename T>
using is_class_t = typename std::is_class<T>::type;

template<typename T>
constexpr auto is_class_v = std::is_class<T>::value;

template<typename T>
using is_enum_t = typename std::is_enum<T>::type;

template<typename T>
constexpr auto is_enum_v = std::is_enum<T>::value;

template<typename T>
using is_const_t = typename std::is_const<T>::type;

template<typename T>
constexpr auto is_const_v = std::is_const<T>::value;

template<typename T>
using is_reference_t = typename std::is_reference<T>::type;

template<typename T>
constexpr auto is_reference_v = std::is_reference<T>::value;

template<typename T>
using is_lvalue_reference_t = typename std::is_lvalue_reference<T>::type;

template<typename T>
constexpr auto is_lvalue_reference_v = std::is_lvalue_reference<T>::value;

template<typename T>
using is_rvalue_reference_t = typename std::is_rvalue_reference<T>::type;

template<typename T>
constexpr auto is_rvalue_reference_v = std::is_rvalue_reference<T>::value;

template<typename T>
using is_pointer_t = typename std::is_pointer<T>::type;

template<typename T>
constexpr auto is_pointer_v = std::is_pointer<T>::value;

template<typename T>
using is_function_t = typename std::is_function<T>::type;

template<typename T>
constexpr auto is_function_v = std::is_function<T>::value;

template<typename T>
constexpr auto extent_v = std::extent<T>::value;

template<typename L, typename R>
using is_same_t = typename std::is_same<L, R>::type;

template<typename L, typename R>
constexpr auto is_same_v = std::is_same<L, R>::value;

template<bool B, typename T, typename F>
constexpr auto conditional_v = std::conditional_t<B, T, F>::value;

template<typename F, typename T>
constexpr auto is_convertible_v = std::is_convertible<F, T>::value;

template<typename T, typename ...Args>
constexpr auto is_constructible_v = std::is_constructible<T, Args...>::value;

template<typename T>
using is_default_constructible_t = typename std::is_default_constructible<T>::type;

template<typename T>
constexpr auto is_default_constructible_v = std::is_default_constructible<T>::value;

#if __GNUC__ < 5
template<typename T>
using is_trivially_default_constructible_t = typename std::has_trivial_default_constructor<T>::type;
template<typename T>
constexpr auto is_trivially_default_constructible_v = std::has_trivial_default_constructor<T>::value;
#else
template<typename T>
using is_trivially_default_constructible_t = typename std::is_trivially_default_constructible<T>::type;
template<typename T>
constexpr auto is_trivially_default_constructible_v = std::is_trivially_default_constructible<T>::value;
#endif

template<typename T>
using is_move_constructible_t = typename std::is_move_constructible<T>::type;

template<typename T>
constexpr auto is_move_constructible_v = std::is_move_constructible<T>::value;

template<typename T>
constexpr auto is_nothrow_move_constructible_v = std::is_nothrow_move_constructible<T>::value;

template<typename T>
using is_copy_constructible_t = typename std::is_copy_constructible<T>::type;

template<typename T>
constexpr auto is_copy_constructible_v = std::is_copy_constructible<T>::value;

template<typename T>
constexpr auto is_nothrow_copy_constructible_v = std::is_nothrow_copy_constructible<T>::value;

template<typename T>
using is_trivially_destructible_t = typename std::is_trivially_destructible<T>::type;

template<typename T>
constexpr auto is_trivially_destructible_v = std::is_trivially_destructible<T>::value;

template<typename T>
constexpr auto is_member_pointer_v = std::is_member_pointer<T>::value;

template<typename T>
constexpr auto is_member_object_pointer_v = std::is_member_object_pointer<T>::value;

template<typename T>
constexpr auto is_member_function_pointer_v = std::is_member_function_pointer<T>::value;

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

template<typename T, bool = is_pointer_v<T>>
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

template<typename T, bool = is_pointer_v<T>>
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

template<typename T, bool = is_reference_v<T>,
                     bool = is_pointer_v<T>,
                     bool = is_array_v<T>>
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
    constexpr static auto lref = is_lvalue_reference_v<T>;
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
    constexpr static auto extent = extent_v<T>;
    using U = remove_all_cv_t<std::remove_extent_t<T>>;
    using type = U[extent];
};

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T>
using full_decay_t = remove_all_cv_t<std::decay_t<T>>;

//-----------------------------------------------------------------------------------------------------------------------------

template<typename T, bool = is_reference_v<T>,
                     bool = is_pointer_v<T>,
                     bool = is_array_v<T>>
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
         bool = is_const_v<T>,
         bool = is_pointer_v<T>>
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
    std::conditional_t<!is_same_v<T, full_decay_t<Arg>> && is_convertible_v<Arg, T>,
                       std::true_type, std::false_type>
{};

template<typename T, typename ...Args>
using is_converting_constructor_t = typename is_converting_constructor<T, Args...>::type;

template<typename T, typename ...Args>
constexpr auto is_converting_constructor_v = is_converting_constructor<T, Args...>::value;

template<typename T>
struct is_lvalue_const_reference:
    std::conditional_t<is_lvalue_reference_v<T> && is_const_v<std::remove_reference_t<T>>,
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
    is_pointer_v<std::remove_reference_t<T>> &&
    is_class_v<std::remove_reference_t<std::remove_pointer_t<T>>>,
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
         typename = std::enable_if_t<is_enum_v<E> & enable_bitmask_enum_v<E>>>
constexpr E operator|(E lhs, E rhs)
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

template<typename E,
         typename = std::enable_if_t<is_enum_v<E> & enable_bitmask_enum_v<E>>>
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

template<typename T, bool = is_class_v<T>>
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

} // namespace rtti

#endif // MISC_TRAITS_H

