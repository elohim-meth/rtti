#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "metatype.h"
#include "metaerror.h"
#include "variant.h"

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
             typename = enable_if_t<!std::is_same<decay_t<T>, argument>::value>>
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
        return value<T>(is_rvalue_reference_t<T>{});
    }

private:
    template<typename T>
    T value(std::true_type) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{m_typeId};
        if (fromType.isLvalueReference())
            throw bad_argument_cast{"Try to bind lvalue reference to rvalue reference"};

        auto toType = MetaType{metaTypeId<T>()};

        if (fromType.decayId() == toType.decayId())
        {
            if (!MetaType::constCompatible(fromType, toType))
                throw bad_argument_cast{std::string{"Const incompatible types: "} +
                                        fromType.typeName() + " -> " + toType.typeName()};

            Decay *ptr = nullptr;
            if (fromType.isArray())
                ptr = static_cast<Decay*>(m_dataptr);
            else
                ptr = static_cast<Decay*>(m_data);

            return std::move(*ptr);
        }
        else if (fromType.decayId() == metaTypeId<variant>())
        {
            auto *ptr = static_cast<variant*>(m_data);
            return std::move(*ptr).value<T>();
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    template<typename T>
    T value(std::false_type) const
    {
        using Decay = decay_t<T>;

        auto fromType = MetaType{m_typeId};
        auto toType = MetaType{metaTypeId<T>()};

        if (fromType.decayId() == toType.decayId())
        {
            if (!MetaType::constCompatible(fromType, toType))
                throw bad_argument_cast{std::string{"Const incompatible types: "} +
                                        fromType.typeName() + " -> " + toType.typeName()};
            Decay *ptr = nullptr;
            if (fromType.isArray())
                ptr = static_cast<Decay*>(m_dataptr);
            else
                ptr = static_cast<Decay*>(m_data);

            return *ptr;
        }
        else if (fromType.decayId() == metaTypeId<variant>())
        {
            auto *ptr = static_cast<variant*>(m_data);
            return ptr->value<T>();
        }

        throw bad_argument_cast{std::string{"Incompatible types: "} +
                               fromType.typeName() + " -> " + toType.typeName()};
    }

    void *m_data = nullptr;
    void *m_dataptr = &m_data;
    const MetaType_ID m_typeId;
};

}


#endif // ARGUMENT_H

