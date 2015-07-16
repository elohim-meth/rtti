#ifndef C_STRING_H
#define C_STRING_H

#include <string>
#include <bits/functional_hash.h>
#include <type_traits>

template<typename CharT>
class cstring_base
{
public:
    template<std::size_t N>
    constexpr explicit cstring_base(const CharT (&data)[N])
        : m_data{data}, m_length{N}
    {}

    template<typename T,
             typename = typename std::enable_if<std::is_same<T, const CharT*>::value>::type>
    explicit cstring_base(T data)
        : m_data{static_cast<const CharT*>(data)},
          m_length{m_data ? std::char_traits<CharT>::length(m_data) : 0}
    {}

    cstring_base() noexcept = default;
    cstring_base(const cstring_base&) noexcept = default;
    cstring_base(cstring_base&&) noexcept = default;
    cstring_base& operator=(const cstring_base&) noexcept = default;
    cstring_base& operator=(cstring_base&&) noexcept = default;

    bool operator==(const cstring_base &other) const
    {
        return (compare(other) == 0);
    }

    bool operator==(const CharT *other) const
    {
        return (compare(cstring_base{other}) == 0);
    }

    bool operator!=(const cstring_base &other) const
    {
        return !(*this == other);
    }

    bool operator!=(const CharT *other) const
    {
        return !(*this == cstring_base{other});
    }

    bool operator<(const cstring_base &other) const
    {
        return (compare(other) < 0);
    }

    bool operator<(const CharT *other) const
    {
        return (compare(cstring_base{other}) < 0);
    }

    bool operator<=(const cstring_base &other) const
    {
        return (compare(other) <= 0);
    }

    bool operator<=(const CharT *other) const
    {
        return (compare(cstring_base{other}) <= 0);
}

    bool operator>(const cstring_base &other) const
    {
        return (compare(other) > 0);
    }

    bool operator>(const CharT *other) const
    {
            return (compare(cstring_base{other}) > 0);
    }

    bool operator>=(const cstring_base &other) const
    {
        return (compare(other) >= 0);
    }

    bool operator>=(const CharT *other) const
    {
            return (compare(cstring_base{other}) >= 0);
    }

    const CharT* data() const noexcept
    {
        return m_data;
    }

    std::size_t length() const noexcept
    {
        return m_length;
    }

    explicit operator bool() const noexcept
    {
        return (m_data != nullptr);
    }
private:
    int compare(const cstring_base &other) const
    {
        if (m_length < other.m_length)
            return -1;
        if (m_length > other.m_length)
            return 1;
        if (m_length == 0)
            return 0;
        return std::char_traits<CharT>::compare(m_data, other.m_data, m_length);
    }

    const CharT *m_data = nullptr;
    std::size_t m_length = 0;
};

namespace std
{
template<typename CharT>
struct hash<cstring_base<CharT>>: public std::__hash_base<std::size_t, cstring_base<CharT>>
{
    using this_t = hash<cstring_base<CharT>>;
    typename this_t::result_type operator()(const typename this_t::argument_type &value) const
    {
        if (!value)
            return 0;

        return _Hash_impl::hash(value.data(), value.length());
    }
};

} // std

using CString = cstring_base<char>;
using WCStrin = cstring_base<wchar_t>;
using CString_16 = cstring_base<char16_t>;
using CString_32 = cstring_base<char32_t>;

#endif // CSTRING_H

