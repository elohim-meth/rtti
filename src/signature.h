#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <function_traits.h>
#include <typelist.h>
#include <typename.h>

#include <sstream>

namespace rtti {

template<typename ...Args>
struct signature
{
    static std::string get(std::string_view const &name)
    {
        return get(name, argument_indexes_t{});
    }

private:
    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<mpl::type_list<Args...>, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args...>;

    template<std::size_t ...I>
    static std::string get(std::string_view const &name, mpl::index_sequence<I...>)
    {
        constexpr auto size = sizeof...(I);
        std::ostringstream os;
        os << name << "(";
        EXPAND (
            os << mpl::type_name<argument_get_t<I>>() << (I < size - 1 ? ", " : "")
        );
        os << ")";
        return os.str();
    }
};

template<typename ...Args>
struct signature<mpl::type_list<Args...>>: signature<Args...>
{};

template<typename F>
struct f_signature: signature<
    typename mpl::function_traits<
        typename mpl::function_traits<F>::uniform_signature
    >::args>
{};

} //namespace rtti

#endif // SIGNATURE_H

