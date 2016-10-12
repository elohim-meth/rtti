#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "metaerror.h"
#include "variant.h"

#include <finally.h>

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
        {
            m_type.destroy(m_buffer);
            m_type.deallocate(m_buffer);
        }
    }

    template<typename T,
             typename = std::enable_if_t<!is_same_v<std::decay_t<T>, argument>>>
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
            std::conditional_t<is_rvalue_reference_v<T>,        std::integral_constant<int, 0>,
            std::conditional_t<is_lvalue_const_reference_v<T>,  std::integral_constant<int, 1>,
            std::conditional_t<is_lvalue_reference_v<T>,        std::integral_constant<int, 2>,
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
        using Decay = std::decay_t<T>;

        auto toType = MetaType{metaTypeId<T>()};
        auto *ptr = const_cast<void*>(m_type.isArray() && !toType.isArray() ? &m_data : m_data);

        if ((m_type.decayId() == toType.decayId()) && MetaType::compatible(m_type, toType))
            return std::move(*static_cast<Decay*>(ptr));
        else if (isVariant())
        {
            auto *v = static_cast<variant*>(ptr);
            if (variant::metafunc_is<T>::invoke(*v, typeId()))
                return std::move(*v).value<Decay>();
        }
        else
        {
            auto fromType = MetaType{typeId()};
            if (MetaType::hasConverter(fromType, toType))
            {
                assert(!m_buffer);
                m_buffer = toType.allocate();
                FINALLY_NAME(freeOnExcept) {
                    toType.deallocate(m_buffer);
                };

                if (isVariant())
                    ptr = static_cast<variant*>(ptr)->raw_data_ptr();
                if (MetaType::convert(ptr, fromType, m_buffer, toType))
                {
                    freeOnExcept.dismiss();
                    return std::move(*static_cast<Decay*>(m_buffer));
                }

                throw bad_variant_convert{std::string{"Conversion failed: "} +
                                          fromType.typeName() + " -> " + toType.typeName()};
            }
        }
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               m_type.typeName() + " -> " + toType.typeName()};
    }

    // lvalue const reference
    template<typename T>
    T value(std::integral_constant<int, 1>) const
    {
        using Decay = std::decay_t<T>;

        auto toType = MetaType{metaTypeId<T>()};
        auto *ptr = const_cast<void*>(m_type.isArray() && !toType.isArray() ? &m_data : m_data);

        if ((m_type.decayId() == toType.decayId()) && MetaType::compatible(m_type, toType))
            return *static_cast<Decay*>(ptr);
        else if (isVariant())
        {
            auto *v = static_cast<variant const*>(ptr);
            if (variant::metafunc_is<T>::invoke(*v, typeId()))
                return v->value<Decay>();
        }
        else
        {
            auto fromType = MetaType{typeId()};
            if (MetaType::hasConverter(fromType, toType))
            {
                assert(!m_buffer);
                m_buffer = toType.allocate();
                FINALLY_NAME(freeOnExcept) {
                    toType.deallocate(m_buffer);
                };

                if (isVariant())
                    ptr = static_cast<variant*>(ptr)->raw_data_ptr();
                if (MetaType::convert(ptr, fromType, m_buffer, toType))
                {
                    freeOnExcept.dismiss();
                    return *static_cast<Decay*>(m_buffer);
                }

                throw bad_variant_convert{std::string{"Conversion failed: "} +
                                          fromType.typeName() + " -> " + toType.typeName()};
            }
        }
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               m_type.typeName() + " -> " + toType.typeName()};
    }

    // lvalue reference
    template<typename T>
    T value(std::integral_constant<int, 2>) const
    {
        using Decay = std::decay_t<T>;

        auto toType = MetaType{metaTypeId<T>()};
        auto *ptr = const_cast<void*>(m_type.isArray() && !toType.isArray() ? &m_data : m_data);

        if ((m_type.decayId() == toType.decayId()) && MetaType::compatible(m_type, toType))
            return *static_cast<Decay*>(ptr);
        else if (isVariant())
        {
            auto *v = static_cast<variant*>(ptr);
            if (variant::metafunc_is<T>::invoke(*v, typeId()))
                return v->value<Decay>();
        }
        auto fromType = MetaType{typeId()};
        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    // no reference
    template<typename T>
    T value(std::integral_constant<int, 3>) const
    {
        using Decay = std::decay_t<T>;

        auto toType = MetaType{metaTypeId<T>()};
       auto *ptr = const_cast<void*>(m_type.isArray() && !toType.isArray() ? &m_data : m_data);

        if ((m_type.decayId() == toType.decayId()) && MetaType::compatible(m_type, toType))
            return internal::move_or_copy<Decay>(ptr, !m_type.isLvalueReference());
        else if (isVariant())
        {
            auto *v = static_cast<variant*>(ptr);
            return v->to<Decay>();
        }
        else if (MetaType::hasConverter(m_type, toType))
        {
            std::aligned_storage_t<sizeof(Decay), alignof(Decay)> buffer;
            if (MetaType::convert(ptr, m_type, &buffer, toType))
            {
                FINALLY{ toType.destroy(&buffer); };
                return internal::move_or_copy<Decay>(&buffer, true);
            }
            throw bad_variant_convert{std::string{"Conversion failed: "} +
                                      m_type.typeName() + " -> " + toType.typeName()};
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               m_type.typeName() + " -> " + toType.typeName()};
    }

    void *m_data = nullptr;
    mutable void *m_buffer = nullptr;
    MetaType m_type = {};
};

}


#endif // ARGUMENT_H

