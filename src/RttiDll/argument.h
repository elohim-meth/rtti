#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "metaerror.h"

#include <type_traits>
#include <utility>

#include "global.h"

namespace rtti {

// forward
class variant;

class DLL_PUBLIC argument final
{
public:
    argument() noexcept = default;
    argument(const variant &value) noexcept;

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<internal::decay_t<T>, variant>::value &&
                 !std::is_same<internal::decay_t<T>, argument>::value>
             ::type>
    argument(T &&value) noexcept
        : m_array{std::is_array<typename std::remove_reference<T>::type>::value},
          m_data{const_cast<void*>(reinterpret_cast<const void*>(std::addressof(value)))},
          m_type{metaTypeId<internal::full_decay_t<T>>()}
    {
//         static_assert(!std::is_array<typename std::remove_reference<T>::type>::value,
//                       "Array types are not supported! "
//                       "Use std::array or explicitly convert to pointer type");
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
        if (empty() || (metaTypeId<internal::full_decay_t<T>>() != m_type))
            throw bad_argument_cast{"Types doesn't match"};

        internal::decay_t<T> *ptr = nullptr;
        if (m_array)
            ptr = reinterpret_cast<internal::decay_t<T>*>(m_dataptr);
        else
            ptr = reinterpret_cast<internal::decay_t<T>*>(m_data);

        return std::move(*ptr);
    }

    template<typename T>
    T value(std::false_type) const
    {
        if (empty() || (metaTypeId<internal::full_decay_t<T>>() != m_type))
            throw bad_argument_cast{"Types doesn't match"};

        internal::decay_t<T> *ptr = nullptr;
        if (m_array)
            ptr = reinterpret_cast<internal::decay_t<T>*>(m_dataptr);
        else
            ptr = reinterpret_cast<internal::decay_t<T>*>(m_data);
        return *ptr;
    }

    bool m_array = false;
    void *m_data = nullptr;
    void *m_dataptr = &m_data;
    const MetaType_ID m_type;
};

}


#endif // ARGUMENT_H

