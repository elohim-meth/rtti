#ifndef ARGUMENT_IMPL_H
#define ARGUMENT_IMPL_H

namespace rtti {

template<typename T, typename>
argument::argument(T &&value) noexcept
    : m_rvalue(!std::is_reference_v<T>)
    , m_value{std::ref(value)}
{}

// rvalue reference
template<typename T>
T argument::value(std::integral_constant<int, 0>) const
{
    using namespace std::literals;
    using Decay = std::decay_t<T>;

    if (!m_rvalue)
        throw bad_variant_cast{"Incompatible argument cast from LValue to RValue reference"};

    auto *value = &m_value;
    if (isVariant())
        value = &m_value.ref<variant>();

#if defined (DEBUG)
    auto fromType = MetaType{value->typeId()};
    auto toType = metaType<Decay>();
#endif

    if (auto *data = value->data<Decay>())
        return std::move(*data);

    m_dummy = value->to<Decay>();
    auto *ptr = m_dummy.raw_data_ptr();
    return std::move(*static_cast<Decay*>(const_cast<void*>(ptr)));
}

// lvalue const reference
template<typename T>
T argument::value(std::integral_constant<int, 1>) const
{
    using Decay = std::decay_t<T>;

    auto const *value = &m_value;
    if (isVariant())
        value = &m_value.cref<variant>();

#if defined (DEBUG)
    auto fromType = MetaType{value->typeId()};
    auto toType = metaType<Decay>();
#endif
    if (auto *data = value->data<Decay>())
        return *data;

    m_dummy = value->to<Decay>();
    auto *ptr = m_dummy.raw_data_ptr();
    return *static_cast<Decay const*>(ptr);
}

// lvalue reference
template<typename T>
T argument::value(std::integral_constant<int, 2>) const
{
    using namespace std::literals;
    using Decay = std::decay_t<T>;

    if (m_rvalue)
        throw bad_variant_cast{"Incompatible argument cast from RValue to non cost LValue reference"};

    auto *value = &m_value;
    if (isVariant())
        value = &m_value.ref<variant>();

#if defined (DEBUG)
    auto fromType = MetaType{value->typeId()};
    auto toType = metaType<Decay>();
#endif

    return value->ref<Decay>();
}

// no reference
template<typename T>
T argument::value(std::integral_constant<int, 3>) const
{
    using namespace std::literals;
    using Decay = std::decay_t<T>;

    auto const *value = &m_value;
    if (isVariant())
        value = &m_value.cref<variant>();

#if defined (DEBUG)
    auto fromType = MetaType{value->typeId()};
    auto toType = metaType<Decay>();
#endif

    return value->to<Decay>();
}

template<typename T>
T argument::value() const
{
    using tag_t =
        std::conditional_t<std::is_rvalue_reference_v<T>,   std::integral_constant<int, 0>,
        std::conditional_t<is_lvalue_const_reference_v<T>,  std::integral_constant<int, 1>,
        std::conditional_t<std::is_lvalue_reference_v<T>,   std::integral_constant<int, 2>,
                                                            std::integral_constant<int, 3>
    >>>;

    if (empty())
        throw bad_argument_cast{"Empty argument"};
    return value<T>(tag_t{});
}

} // namespace rtti


#endif // ARGUMENT_IMPL_H
