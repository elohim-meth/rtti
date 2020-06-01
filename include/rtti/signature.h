#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <rtti/function_traits.h>
#include <rtti/typelist.h>
#include <rtti/typename.h>

#include <sstream>

namespace rtti {

template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
class prefix_ostream_iterator: public std::iterator<std::output_iterator_tag, T>
{
private:
    using stream_t = std::basic_ostream<CharT, Traits>;
    using string_view_t = std::basic_string_view<CharT, Traits>;

    stream_t &m_stream;
    string_view_t m_prefix;
    bool first = true;
public:
    using char_type = CharT;
    using traits_type = Traits;
    using ostream_type = std::basic_ostream<CharT, Traits>;

    prefix_ostream_iterator(stream_t& stream, string_view_t prefix = "")
    :
        m_stream{stream},
        m_prefix{prefix}
    {}

    prefix_ostream_iterator& operator*()       { return *this; }
    prefix_ostream_iterator& operator++()      { return *this; }
    prefix_ostream_iterator& operator++(int)   { return *this; }

    stream_t& stream() { return m_stream; }

    prefix_ostream_iterator& operator=(T const &value)
    {
        if (first)
        {
            m_stream << value;
            first = false;
        }
        else
            m_stream << m_prefix << value;

        return *this;
    }
};

template<typename ...Args>
struct signature
{
    static std::string get(std::string_view name)
    {
        std::ostringstream os;
        auto it = prefix_ostream_iterator<std::string_view>{os, ", "};
        os << name << '(';
        (it = ... = type_name<Args>());
        os << ')';
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

