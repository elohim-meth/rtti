#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "metaerror.h"
#include "variant.h"

#include <utility>

#include "global.h"

namespace rtti {

class DLL_PUBLIC argument final
{
public:
    argument() = default;
    argument(argument const&) = delete;
    argument& operator=(argument const&) = delete;
    argument(argument&&) = default;
    argument& operator=(argument&&) = delete;

    template<typename T,
             typename = enable_if_t<!std::is_same<decay_t<T>, argument>::value>>
    argument(T &&value) noexcept
        : m_data{const_cast<void*>(reinterpret_cast<void const*>(std::addressof(value)))},
          m_type{metaTypeId<T>()}
    {}

    bool empty() const
    { return m_data == nullptr; }
    MetaType_ID typeId() const;

    template<typename T,
             typename U = conditional_t<is_rvalue_reference_t<T>::value, remove_reference_t<T>, T>>
    U value() const
    {
        if (empty())
            throw bad_argument_cast{"Empty argument"};
        return value<U>(is_lvalue_reference_t<T>{});
    }

private:
    bool isVariant() const;

    // no reference or rvalue reference
    template<typename T>
    T value(std::false_type) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{typeId()};
        auto toType = MetaType{metaTypeId<T>()};

        if (MetaType::compatible(fromType, toType))
        {
            if (m_type.decayId() == toType.decayId())
            {
                Decay *ptr = nullptr;
                if (fromType.isArray())
                    ptr = static_cast<Decay*>(m_dataptr);
                else
                    ptr = static_cast<Decay*>(m_data);

                if (m_type.isLvalueReference())
                    return *ptr;
                else
                    return std::move(*ptr);
            }
            else if (isVariant())
            {
                auto *ptr = static_cast<variant*>(m_data);
                if (m_type.isLvalueReference())
                    return ptr->to<Decay>();
                else
                    return std::move(*ptr).value<Decay>();
            }
        }

        if (MetaType::hasConverter(fromType, toType))
        {
            alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
            if (MetaType::convert(m_data, fromType, &buffer, toType))
                return std::move(*reinterpret_cast<T*>(&buffer));

            throw bad_variant_convert{std::string{"Conversion failed: "} +
                                      fromType.typeName() + " -> " + toType.typeName()};
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    // lvalue reference
    template<typename T>
    T value(std::true_type) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{typeId()};
        auto toType = MetaType{metaTypeId<T>()};

        if (MetaType::compatible(fromType, toType))
        {
            if (m_type.decayId() == toType.decayId())
            {
                Decay *ptr = nullptr;
                if (fromType.isArray())
                    ptr = static_cast<Decay*>(m_dataptr);
                else
                    ptr = static_cast<Decay*>(m_data);

                return *ptr;
            }
            else if (isVariant())
            {
                auto *ptr = static_cast<variant*>(m_data);
                return ptr->value<T>();
            }
        }
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    void *m_data = nullptr;
    void *m_dataptr = &m_data;
    MetaType m_type;
};

}


#endif // ARGUMENT_H

