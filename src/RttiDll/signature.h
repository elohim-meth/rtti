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
    static std::string get(const char *name)
    {
        return signature_imp(name, argument_indexes_t{});
    }

private:
    template<std::size_t I>
    using argument_get_t = typelist_get_t<type_list<Args...>, I>;
    using argument_indexes_t = typename index_sequence_for<Args...>::type;

    template<std::size_t ...I>
    static std::string signature_imp(const char *name, index_sequence<I...>)
    {
        constexpr auto size = sizeof...(I);
        std::ostringstream os;
        os << (name ? name : "")  << "(";
        EXPAND (
            os << type_name<argument_get_t<I>>() << (I < size - 1 ? ", " : "")
        );
        os << ")";
        return os.str();
    }
};

template<typename ...Args>
struct signature<type_list<Args...>>: signature<Args...>
{};

template<typename F>
struct f_signature: signature<typename function_traits<typename function_traits<F>::uniform_signature>::args>
{};

} //namespace rtti

#endif // SIGNATURE_H

