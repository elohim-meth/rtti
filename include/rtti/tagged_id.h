#ifndef TAGGED_ID_H
#define TAGGED_ID_H

#include <functional>

namespace mpl {

template<typename Tag, typename Type, Type default_value>
class ID
{
public:
    using type = Type;
    static constexpr Type Default = default_value;

    constexpr ID() noexcept
        : m_val(default_value)
    {}

    explicit constexpr ID(Type val) noexcept
        : m_val(val)
    {}

    explicit constexpr operator Type() const noexcept
    {
        return m_val;
    }

    constexpr Type value() const noexcept
    {
        return m_val;
    }

    constexpr bool operator==(ID const &value) const noexcept
    {
        return m_val == value.m_val;
    }

    constexpr bool operator!=(ID const &value) const noexcept
    {
        return !(*this == value);
    }

    constexpr bool valid() const noexcept
    {
        return m_val != Default;
    }
private:
    Type m_val;
};

} // namespace mpl

namespace std
{

template<typename Tag, typename Type, Type default_value>
struct hash<mpl::ID<Tag, Type, default_value>>
{
    using result_type = std::size_t;
    using argument_type = mpl::ID<Tag, Type, default_value>;

    result_type operator()(argument_type const &id) const
    {
        return std::hash<Type>(id.value());
    }
};

} // namespace std
#endif //TAGGED_ID_H
