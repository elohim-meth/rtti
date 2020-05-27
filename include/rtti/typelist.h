#ifndef TYPELIST_H
#define TYPELIST_H

#include <tuple>
#include <type_traits>
#include <cstdint>

namespace mpl {

template <typename ...Args>
struct type_list
{};

using empty_list = type_list<>;

// identity
template<typename T>
struct identity
{
    using type = T;
};

// is_typelist
template<typename T>
struct is_typelist: std::false_type
{};

template<typename ...Args>
struct is_typelist<type_list<Args...>>: std::true_type
{};

template<typename T>
constexpr auto is_typelist_v = is_typelist<T>::value;

template<typename T>
struct check_typelist
{
    using type = void;
    static constexpr void *value = nullptr;
    static_assert(is_typelist_v<T>, "T is not a type_list!");
};


// size
template<typename T>
struct typelist_size: check_typelist<T>
{};

template<typename ...Args>
struct typelist_size<type_list<Args...>>: std::integral_constant<std::size_t, sizeof...(Args)>
{};

template<typename T>
constexpr auto typelist_size_v = typelist_size<T>::value;

template<typename T>
struct typelist_isempty: check_typelist<T>
{};

template<typename ...Args>
struct typelist_isempty<type_list<Args...>>: std::integral_constant<bool, sizeof...(Args) == 0>
{};

template<typename T>
constexpr auto typelist_isempty_v = typelist_isempty<T>::value;

// first
template<typename T>
struct typelist_first: check_typelist<T>
{};

template<typename H, typename ...T>
struct typelist_first<type_list<H, T...>>
{
    using type = H;
};

template<typename ...T>
struct typelist_first<type_list<T...>>
{
    static_assert(sizeof...(T) != 0, "List is empty!");
};

// last
template<typename T>
struct typelist_last: check_typelist<T>
{};

template<typename H, typename ...T>
struct typelist_last<type_list<H, T...>>: typelist_last<type_list<T...>>
{};

template<typename H>
struct typelist_last<type_list<H>>: identity<H>
{};

template<typename ...T>
struct typelist_last<type_list<T...>>
{
    static_assert(sizeof...(T) != 0, "List is empty!");
};

// get
template<typename T, std::size_t I>
struct typelist_get: check_typelist<T>
{};

template<typename H, typename ...T, std::size_t I>
struct typelist_get<type_list<H, T...>, I>: typelist_get<type_list<T...>, I - 1>
{};

template<typename H, typename ...T>
struct typelist_get<type_list<H, T...>, 0>: identity<H>
{};

template<typename ...T, std::size_t I>
struct typelist_get<type_list<T...>, I>
{
    static_assert(sizeof...(T) != 0, "Index out of bounds");
};

template<typename T, std::size_t I>
using typelist_get_t = typename typelist_get<T, I>::type;

// indexof
namespace internal {

template<typename T, typename V, std::int64_t index>
struct typelist_indexof_helper: check_typelist<T>
{};

template<typename H, typename ...T, typename V, std::int64_t index>
struct typelist_indexof_helper<type_list<H, T...>, V, index>:
        std::conditional_t<std::is_same_v<H, V>,
            std::integral_constant<std::int64_t, index>,
            typelist_indexof_helper<type_list<T...>, V, index + 1>
        >
{};

template<typename V, std::int64_t index>
struct typelist_indexof_helper<empty_list, V, index>: std::integral_constant<std::int64_t, -1>
{};

}

template<typename T, typename V>
using typelist_indexof = internal::typelist_indexof_helper<T, V, 0>;

template<typename T, typename V>
constexpr auto typelist_indexof_v = typelist_indexof<T, V>::value;

template<typename T, typename V>
struct typelist_exists: std::integral_constant<bool, typelist_indexof_v<T, V> >= 0>
{};

template<typename T, typename V>
constexpr auto typelist_exists_v = typelist_exists<T, V>::value;

// concat
template<typename T1, typename T2>
struct typelist_concat: check_typelist<T1>, check_typelist<T2>
{};

template<typename ...T1, typename ...T2>
struct typelist_concat<type_list<T1...>, type_list<T2...>>: identity<type_list<T1..., T2...>>
{};

// push front
template<typename T, typename ...V>
struct typelist_pushfront: check_typelist<T>
{};

template<typename ...T, typename ...V>
struct typelist_pushfront<type_list<T...>, V...>: identity<type_list<V..., T...>>
{};

// pop front
template<typename T>
struct typelist_popfront: check_typelist<T>
{};

template<typename H, typename ...T>
struct typelist_popfront<type_list<H, T...>>: identity<type_list<T...>>
{};

template<typename ...T>
struct typelist_popfront<type_list<T...>>
{
    static_assert(sizeof...(T) != 0, "List is empty!");
};

// push back
template<typename T, typename ...V>
struct typelist_pushback: check_typelist<T>
{};

template<typename ...T, typename ...V>
struct typelist_pushback<type_list<T...>, V...>: identity<type_list<T..., V...>>
{};

// pop back
namespace internal {

template<typename P, typename T>
struct typelist_popback_helper: check_typelist<T>
{};

template<typename ...P, typename H, typename ...T>
struct typelist_popback_helper<type_list<P...>, type_list<H, T...>>:
        typelist_popback_helper<type_list<P..., H>, type_list<T...>>
{};

template<typename ...P, typename H>
struct typelist_popback_helper<type_list<P...>, type_list<H>>: identity<type_list<P...>>
{};

template<typename P, typename ...T>
struct typelist_popback_helper<P, type_list<T...>>
{
    static_assert(sizeof...(T) != 0, "List is empty!");
};

}

template<typename T>
using typelist_popback = internal::typelist_popback_helper<empty_list, T>;

// remove
namespace internal {

template<typename P, typename T, typename V>
struct typelist_remove_helper: check_typelist<T>
{};

template<typename ...P, typename H, typename ...T, typename V>
struct typelist_remove_helper<type_list<P...>, type_list<H, T...>, V>:
        std::conditional_t<std::is_same_v<H, V>,
            identity<type_list<P..., T...>>,
            typelist_remove_helper<type_list<P..., H>, type_list<T...>, V>
        >
{};

template<typename ...P, typename V>
struct typelist_remove_helper<type_list<P...>, empty_list, V>: identity<type_list<P...>>
{};

}

template<typename T, typename V>
using typelist_remove = internal::typelist_remove_helper<empty_list, T, V>;

template<typename T, typename V>
using typelist_remove_t = typename typelist_remove<T, V>::type;

// remove_all
namespace internal {

template<typename P, typename T, typename V>
struct typelist_removeall_helper: check_typelist<T>
{};

template<typename ...P, typename H, typename ...T, typename V>
struct typelist_removeall_helper<type_list<P...>, type_list<H, T...>, V>:
        std::conditional_t<std::is_same_v<H, V>,
            typelist_removeall_helper<type_list<P...>, type_list<T...>, V>,
            typelist_removeall_helper<type_list<P..., H>, type_list<T...>, V>
        >
{};

template<typename ...P, typename V>
struct typelist_removeall_helper<type_list<P...>, empty_list, V>: identity<type_list<P...>>
{};

}

template<typename T, typename V>
using typelist_removeall = internal::typelist_removeall_helper<empty_list, T, V>;

template<typename T, typename V>
using typelist_removeall_t = typename typelist_removeall<T, V>::type;

// remove_duplicates
namespace internal {

template<typename P, typename T>
struct typelist_remove_duplicates_helper: check_typelist<T>
{};

template<typename ...P, typename H, typename ...T>
struct typelist_remove_duplicates_helper<type_list<P...>, type_list<H, T...>>:
        std::conditional_t<typelist_exists_v<type_list<T...>, H>,
            typelist_remove_duplicates_helper<type_list<P...>, type_list<T...>>,
            typelist_remove_duplicates_helper<type_list<P..., H>, type_list<T...>>
        >
{};

template<typename ...P>
struct typelist_remove_duplicates_helper<type_list<P...>, empty_list>: identity<type_list<P...>>
{};

}

template<typename T>
using typelist_remove_duplicates = internal::typelist_remove_duplicates_helper<empty_list, T>;

template<typename T>
using typelist_remove_duplicates_t = typename typelist_remove_duplicates<T>::type;

// unordered_equal
template<typename T1, typename T2>
struct typelist_unorderedequal: check_typelist<T1>, check_typelist<T2>
{};

template<typename H1, typename ...T1, typename ...T2>
struct typelist_unorderedequal<type_list<H1, T1...>, type_list<T2...>>:
        std::conditional_t<typelist_exists_v<type_list<T2...>, H1>,
            typelist_unorderedequal<type_list<T1...>, typelist_remove_t<type_list<T2...>, H1>>,
            std::false_type
        >
{};

template<typename H, typename ...T>
struct typelist_unorderedequal<type_list<H, T...>, empty_list>: std::false_type
{};

template<typename H, typename ...T>
struct typelist_unorderedequal<empty_list, type_list<H, T...>>: std::false_type
{};

template<>
struct typelist_unorderedequal<empty_list, empty_list>: std::true_type
{};

template<typename T1, typename T2>
constexpr auto typelist_unorderedequal_v = typelist_unorderedequal<T1, T2>::value;

// convert to tuple
namespace internal {

template<typename T>
struct typelist_runtime_helper: identity<T>
{};

template<typename ...T>
struct typelist_runtime_helper<type_list<T...>>:
        identity<std::tuple<typename typelist_runtime_helper<T>::type...>>
{};

}

template<typename T>
struct typelist_tuple: check_typelist<T>
{};

template<typename ...T>
struct typelist_tuple<type_list<T...>>: internal::typelist_runtime_helper<type_list<T...>>
{};

// map
// Applies a given transformation function F to a type list,
// returning the sequence of applications
// F: The transformation function. It should be a unary type function enity.
template<typename T, template<typename> class F>
struct typelist_map: check_typelist<T>
{};

template<typename ...T, template<typename> class F>
struct typelist_map<type_list<T...>, F>: identity<type_list<typename F<T>::type...>>
{};

template<typename T, template<typename> class F>
using typelist_map_t = typename typelist_map<T, F>::type;

// filter
// Filters the contents of an input type list based on a boolean predicate filter F.
// F: The filtering function. It should be a unary bool value function enity.
namespace internal {

template<template<typename> class F, typename P, typename T>
struct typelist_filter_helper: check_typelist<T>
{};

template<template<typename> class F, typename ...P, typename H, typename ...T>
struct typelist_filter_helper<F, type_list<P...>, type_list<H, T...>>:
        std::conditional_t<F<H>::value,
            typelist_filter_helper<F, type_list<P..., H>, type_list<T...>>,
            typelist_filter_helper<F, type_list<P...>, type_list<T...>>
        >
{};

template<template<typename> class F, typename ...P>
struct typelist_filter_helper<F, type_list<P...>, empty_list>: identity<type_list<P...>>
{};

}

template<typename T, template<typename> class F>
using typelist_filter = internal::typelist_filter_helper<F, empty_list, T>;

// foldl
/*
 * Processes an input sequence generating a result value through recursive
 * applications of a given composition function F.
 * foldl applies the recursion on the left side of the composition operation.
 *
 *  F: The composition function. It should be a binary type function enity, where the
 *     left argumment is I (state) of the computation, and
 *     right argumentis the current element of the sequence.
 *  I: Initial state of the computation.
 *  T: The sequence, represented as a type_list.
*/
template<typename T, template<typename, typename> class F, typename I>
struct typelist_foldl: check_typelist<T>
{};

template<typename H, typename ...T, template<typename, typename> class F, typename I>
struct typelist_foldl<type_list<H, T...>, F, I>:
        typelist_foldl<type_list<T...>, F, typename F<I, H>::type>
{};

template<template<typename, typename> class F, typename I>
struct typelist_foldl<empty_list, F, I>: identity<I>
{};

// foldr
/*
* Processes an input sequence generating a result value through recursive
* applications of a given composition function F.
* foldr applies the recursion on the right side of the composition operation.
*
*  F: The composition function. It should be a binary type function enity, where the
*     left argument is the current element of the sequence, and
*     right argumment is I (state) of the computation
*  I: Initial state of the computation.
*  T: The sequence, represented as a type_list.
*/

template<typename T, template<typename, typename> class F, typename I>
struct typelist_foldr: check_typelist<T>
{};

template<typename H, typename ...T, template<typename, typename> class F, typename I>
struct typelist_foldr<type_list<H, T...>, F, I>:
        F<H, typename typelist_foldr<type_list<T...>, F, I>::type>::type
{};

template<template<typename, typename> class F, typename I>
struct typelist_foldr<empty_list, F, I>: identity<I>
{};

// index sequence
template <std::size_t ...Ints>
struct index_sequence {
    using size = std::integral_constant<std::size_t, sizeof...(Ints)>;
};

template <std::size_t N, std::size_t ...Ints>
struct make_index_sequence: make_index_sequence<N - 1, N - 1, Ints...>
{};

template <std::size_t ...Ints>
struct make_index_sequence<0, Ints...>: identity<index_sequence<Ints...>>
{};

template<typename ...Args>
struct index_sequence_for: make_index_sequence<sizeof...(Args)>
{};

template<typename ...Args>
struct index_sequence_for<type_list<Args...>>: index_sequence_for<Args...>
{};

template<typename ...Args>
struct index_sequence_for<std::tuple<Args...>>: index_sequence_for<Args...>
{};

template<typename ...Args>
using index_sequence_for_t = typename index_sequence_for<Args...>::type;

namespace internal {

struct expand
{
    expand(std::initializer_list<int>&&) {}
};

}

} // namespace mpl
#define EXPAND(CODE) \
    mpl::internal::expand{0, ((CODE),0)...};

#endif // TYPELIST_H

