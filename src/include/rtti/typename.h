#ifndef TYPENAME_H
#define TYPENAME_H

#include <string>
#include <cstring>

namespace mpl {

namespace internal {

template<typename T>
class TypeName final
{
public:
    TypeName()
    {
        char const *prettyFunc = __PRETTY_FUNCTION__;
        char const *begin = std::strstr(prettyFunc, "T =");
        if (begin) {
            begin += sizeof("T =");
            char const *end = std::strrchr(begin, ']');
            if (end)
                m_signature = std::string{begin, end};
        }
    }
    std::string const& signature() const noexcept
    { return m_signature; }
private:
    std::string m_signature;
};

}

// More precise version!
// C++11 states that initialization of local statics is thread-safe
// and constructor is called only once. Using __PRETTY_FUNCTION__ hack.
template<typename T>
std::string const& type_name()
{
    static internal::TypeName<T> const demangler;
    return demangler.signature();
}

} // namespace mpl

#endif // TYPENAME_H
