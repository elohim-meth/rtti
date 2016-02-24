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
    argument() noexcept = default;
    argument(argument const&) = delete;
    argument& operator=(argument const&) = delete;
    argument(argument&&) noexcept = default;
    argument& operator=(argument&&) = delete;
    ~argument() noexcept
    {
        if (m_buffer)
            ::operator delete(m_buffer);
    }

    template<typename T,
             typename = enable_if_t<!std::is_same<decay_t<T>, argument>::value>>
    argument(T &&value) noexcept
        : m_data{const_cast<void*>(reinterpret_cast<void const*>(std::addressof(value)))},
          m_type{metaTypeId<T>()}
    {}

    bool empty() const noexcept
    { return m_data == nullptr; }
    MetaType_ID typeId() const;

    template<typename T>
    T value() const
    {
        using tag_t =
            conditional_t<is_rvalue_reference_t<T>::value, std::integral_constant<int, 0>,
            conditional_t<is_lvalue_const_reference_t<T>::value, std::integral_constant<int, 1>,
            conditional_t<is_lvalue_reference_t<T>::value, std::integral_constant<int, 2>,
                                                           std::integral_constant<int, 3>
            >>>;
        if (empty())
            throw bad_argument_cast{"Empty argument"};
        return value<T>(tag_t{});
    }

private:
    bool isVariant() const;

    // rvalue reference
    template<typename T>
    T value(std::integral_constant<int, 0>) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{typeId()};
        auto toType = MetaType{metaTypeId<T>()};

        if (MetaType::compatible(fromType, toType))
        {
            auto *ptr = m_type.isArray() ? m_dataptr : m_data;
            if (m_type.decayId() == toType.decayId())
                return std::move(*static_cast<Decay*>(ptr));
            else if (isVariant())
            {
                auto *v = static_cast<variant*>(ptr);
                return std::move(*v).value<Decay>();
            }
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    // lvalue const reference
    template<typename T>
    T value(std::integral_constant<int, 1>) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{typeId()};
        auto toType = MetaType{metaTypeId<T>()};

        if (MetaType::compatible(fromType, toType))
        {
            auto *ptr = m_type.isArray() ? m_dataptr : m_data;
            if (m_type.decayId() == toType.decayId())
                return *static_cast<Decay*>(ptr);
            else if (isVariant())
            {
                auto *v = static_cast<variant const*>(ptr);
                return v->value<T>();
            }
        }

        if (MetaType::hasConverter(m_type, toType))
        {
            assert(!m_buffer);
            m_buffer = ::operator new(sizeof(Decay));
            auto *ptr = m_type.isArray() ? m_dataptr : m_data;
            if (isVariant())
                ptr = static_cast<variant*>(ptr)->raw_data_ptr();
            if (MetaType::convert(ptr, fromType, m_buffer, toType))
                return *static_cast<Decay*>(m_buffer);

            throw bad_variant_convert{std::string{"Conversion failed: "} +
                                      m_type.typeName() + " -> " + toType.typeName()};
        }
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    // lvalue reference
    template<typename T>
    T value(std::integral_constant<int, 2>) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{typeId()};
        auto toType = MetaType{metaTypeId<T>()};

        if (MetaType::compatible(fromType, toType))
        {
            auto *ptr = m_type.isArray() ? m_dataptr : m_data;

            if (m_type.decayId() == toType.decayId())
                return *static_cast<Decay*>(ptr);
            else if (isVariant())
            {
                auto *v = static_cast<variant*>(ptr);
                return v->value<T>();
            }
        }
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    // no reference
    template<typename T>
    T value(std::integral_constant<int, 3>) const
    {
        using Decay = decay_t<T>;

        auto toType = MetaType{metaTypeId<T>()};
        auto *ptr = m_type.isArray() ? m_dataptr : m_data;

        if (MetaType::compatible(m_type, toType) && (m_type.decayId() == toType.decayId()))
            return internal::copy_or_move<Decay>(ptr, !m_type.isLvalueReference());
        else if (isVariant())
        {
            auto *v = static_cast<variant*>(ptr);
            return v->to<Decay>();
        }
        else if (MetaType::hasConverter(m_type, toType))
        {
            alignas(Decay) std::uint8_t buffer[sizeof(Decay)] = {0};
            if (MetaType::convert(ptr, m_type, &buffer, toType))
                return internal::copy_or_move<Decay>(&buffer, true);

            throw bad_variant_convert{std::string{"Conversion failed: "} +
                                      m_type.typeName() + " -> " + toType.typeName()};
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               m_type.typeName() + " -> " + toType.typeName()};
    }

    void *m_data = nullptr;
    void *m_dataptr = &m_data;
    mutable void *m_buffer = nullptr;
    MetaType m_type;
};

}


#endif // ARGUMENT_H

