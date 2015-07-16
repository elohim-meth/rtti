#ifndef C_STRING_H
#define C_STRING_H

#include <string>
#include <bits/functional_hash.h>
#include <type_traits>

class CString
{
public:
    template<std::size_t N>
    constexpr explicit CString(const char (&data)[N])
        : m_data{data}, m_length{N}
    {}

    template<typename T,
             typename = typename std::enable_if<std::is_same<T, const char*>::value>::type>
    explicit CString(T data)
        : m_data{static_cast<const char*>(data)},
          m_length{m_data ? std::char_traits<char>::length(m_data) : 0}
    {}

    CString() noexcept = default;
    CString(const CString&) noexcept = default;
    CString(CString&&) noexcept = default;
    CString& operator=(const CString&) noexcept = default;
    CString& operator=(CString&&) noexcept = default;

    bool operator==(const CString &other) const
    {
        return (compare(other) == 0);
    }

    bool operator==(const char *other) const
    {
        return (compare(CString{other}) == 0);
    }

    bool operator!=(const CString &other) const
    {
        return !(*this == other);
    }

    bool operator!=(const char *other) const
    {
        return !(*this == CString{other});
    }

    bool operator<(const CString &other) const
    {
        return (compare(other) < 0);
    }

    bool operator<(const char *other) const
    {
        return (compare(CString{other}) < 0);
    }

    bool operator<=(const CString &other) const
    {
        return (compare(other) <= 0);
    }

    bool operator<=(const char *other) const
    {
        return (compare(CString{other}) <= 0);
}

    bool operator>(const CString &other) const
    {
        return (compare(other) > 0);
    }

    bool operator>(const char *other) const
    {
            return (compare(CString{other}) > 0);
    }

    bool operator>=(const CString &other) const
    {
        return (compare(other) >= 0);
    }

    bool operator>=(const char *other) const
    {
            return (compare(CString{other}) >= 0);
    }

    const char* data() const noexcept
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
    int compare(const CString &other) const
    {
        if (m_length < other.m_length)
            return -1;
        if (m_length > other.m_length)
            return 1;
        if (m_length == 0)
            return 0;
        return std::char_traits<char>::compare(m_data, other.m_data, m_length);
    }

    const char *m_data = nullptr;
    std::size_t m_length = 0;
};

namespace std
{
template<>
struct hash<CString>: public std::__hash_base<std::size_t, CString>
{
    result_type operator()(const argument_type &value) const
    {
        if (!value)
            return 0;

        return _Hash_impl::hash(value.data(), value.length());
    }
};

} // std

#endif // CSTRING_H

