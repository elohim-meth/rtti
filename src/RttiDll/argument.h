#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "metaerror.h"
#include "variant.h"

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
    argument(const argument&) = delete;
    argument& operator=(const argument&) = delete;
    argument(argument&&) = default;
    argument& operator=(argument&&) = delete;

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<internal::decay_t<T>, argument>::value>
             ::type>
    argument(T &&value) noexcept
        : m_data{const_cast<void*>(reinterpret_cast<const void*>(std::addressof(value)))},
          m_typeId{metaTypeId<T>()}
    {}

    bool empty() const noexcept
    {
        return m_data == nullptr;
    }

    template<typename T>
    T value() const
    {
        if (empty())
            throw bad_argument_cast{"Empty argument"};
        return value<T>(std::is_rvalue_reference<T>{});
    }

private:
    template<typename T>
    T value(std::true_type) const
    {
        using decay_t = internal::decay_t<T>;

        auto fromType = MetaType{m_typeId};
        if (fromType.typeFlags() & MetaType::LvalueReference)
            throw bad_argument_cast{"Try to bind lvalue reference to rvalue reference"};

        auto toTypeId = metaTypeId<internal::full_decay_t<T>>();

        if (fromType.decayId() == toTypeId)
        {
            decay_t *ptr = nullptr;
            if (fromType.typeFlags() & MetaType::Array)
                ptr = static_cast<decay_t*>(m_dataptr);
            else
                ptr = static_cast<decay_t*>(m_data);

            return std::move(*ptr);
        }
        else if (fromType.decayId() == metaTypeId<variant>())
        {
            auto *ptr = static_cast<variant*>(m_data);
            return std::move(*ptr).value<decay_t>();
        }
        throw bad_argument_cast{"Types doesn't match"};
    }

    template<typename T>
    T value(std::false_type) const
    {
        using decay_t = internal::decay_t<T>;

        auto fromType = MetaType{m_typeId};
        auto toTypeId = metaTypeId<internal::full_decay_t<T>>();

        if (fromType.decayId() == toTypeId)
        {
            decay_t *ptr = nullptr;
            if (fromType.typeFlags() & MetaType::Array)
                ptr = static_cast<decay_t*>(m_dataptr);
            else
                ptr = static_cast<decay_t*>(m_data);

            return *ptr;
        }
        else if (fromType.decayId() == metaTypeId<variant>())
        {
            auto *ptr = static_cast<variant*>(m_data);
            return ptr->value<decay_t>();
        }
        throw bad_argument_cast{"Types doesn't match"};
    }

    void *m_data = nullptr;
    void *m_dataptr = &m_data;
    const MetaType_ID m_typeId;
};

}


#endif // ARGUMENT_H

