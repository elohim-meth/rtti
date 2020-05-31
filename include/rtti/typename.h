#ifndef TYPENAME_H
#define TYPENAME_H

#include <string>
#include <cstring>

namespace rtti {

namespace internal {

template<typename T>
class TypeName final
{
public:
    TypeName()
    {
        auto *prettyFunc = __PRETTY_FUNCTION__;
        if (auto *begin = std::strstr(prettyFunc, "T ="))
        {
            begin += sizeof("T =");
            if (auto *end = std::strrchr(begin, ']'))
                m_signature = {begin, static_cast<size_t>(end - begin)};
        }
    }
    auto signature() const noexcept
    { return m_signature; }
private:
    std::string_view m_signature;
};

}

// More precise version!
// C++11 states that initialization of local statics is thread-safe
// and constructor is called only once. Using __PRETTY_FUNCTION__ hack.
template<typename T>
std::string_view type_name()
{
    static internal::TypeName<T> const demangler;
    return demangler.signature();
}

} // namespace mpl

#endif // TYPENAME_H
