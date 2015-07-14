#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "variant.h"

#include <type_traits>
#include <utility>

#include "global.h"

namespace rtti {

class DLL_PUBLIC bad_argument_cast final: public std::bad_cast
{
public:
    const char* what() const noexcept override
    { return "Bad argument cast"; }
};

class DLL_PUBLIC argument final
{
public:
    argument() noexcept = default;

    argument(const variant &value) noexcept
        : m_data{value.raw_data_ptr()}, m_type{value.type()}
    {}

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<internal::decay_t<T>, variant>::value &&
                 !std::is_same<internal::decay_t<T>, argument>::value>
             ::type>
    argument(T &&value) noexcept
        : m_data{std::addressof(value)},
          m_type{metaTypeId<internal::decay_t<T>>()}
    {
         static_assert(!std::is_array<typename std::remove_reference<T>::type>::value,
                       "Array types are not supported! "
                       "Use std::array or explicitly convert to pointer type");
    }

    bool empty() const noexcept
    {
        return m_data == nullptr;
    }

    template<typename T>
    T value() const
    {
        return value<T>(std::is_rvalue_reference<T>{});
    }

private:
    template<typename T>
    T value(std::true_type) const
    {
        if (empty() || (metaTypeId<internal::decay_t<T>>() != m_type))
            throw bad_argument_cast{};
        auto ptr = reinterpret_cast<internal::decay_t<T>*>(
                    const_cast<void*>(m_data));
        return std::move(*ptr);
    }

    template<typename T>
    T value(std::false_type) const
    {
        if (empty() || (metaTypeId<internal::decay_t<T>>() != m_type))
            throw bad_argument_cast{};
        auto ptr = reinterpret_cast<internal::decay_t<T>*>(
                    const_cast<void*>(m_data));
        return *ptr;
    }

    void *m_data = nullptr;
    const MetaType_ID m_type;
};

}


#endif // ARGUMENT_H

