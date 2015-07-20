#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"

#include <type_traits>
#include <utility>
#include <new>

#include "global.h"

namespace rtti {

// forward
class argument;
class variant;

namespace internal {

template<typename T>
using decay_t = typename std::decay<T>::type;

constexpr std::size_t STORAGE_SIZE = sizeof(void*) * 2;

template<typename T,
         bool Small = sizeof(typename std::decay<T>::type) <= STORAGE_SIZE,
         bool Safe = std::is_move_constructible<T>::value>
using is_inplace = std::integral_constant<bool, Small && Safe>;

union DLL_PUBLIC variant_type_storage
{
    alignas(STORAGE_SIZE) std::uint8_t buffer[STORAGE_SIZE];
    void *ptr;
};

struct DLL_PUBLIC variant_function_table
{
    using type_t = MetaType_ID(*)();
    using access_t = void* (*) (const variant_type_storage&);
    using clone_t = void (*) (const variant_type_storage&, variant_type_storage&);
    using move_t = void (*) (variant_type_storage&, variant_type_storage&);
    using destroy_t = void (*) (variant_type_storage&);

    const type_t f_type = nullptr;
    const access_t f_access = nullptr;
    const clone_t f_clone = nullptr;
    const move_t f_move = nullptr;
    const destroy_t f_destroy = nullptr;

    variant_function_table(type_t type,
                       access_t access,
                       clone_t clone,
                       move_t move,
                       destroy_t destroy) noexcept
        : f_type{type},
          f_access{access},
          f_clone{clone},
          f_move{move},
          f_destroy{destroy}
    {}
};

template<typename T, bool = is_inplace<T>::value>
struct function_table_selector;

template<typename T>
struct function_table_selector<T, true>
{
    static MetaType_ID type() noexcept
    {
        return metaTypeId<T>();
    }

    static void* access(const variant_type_storage &value) noexcept
    {
        auto ptr = reinterpret_cast<const T*>(&value.buffer);
        return const_cast<T*>(ptr);
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        auto ptr = reinterpret_cast<const T*>(&src.buffer);
        new (&dst.buffer) T(*ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        auto ptr = reinterpret_cast<T*>(&src.buffer);
        new (&dst.buffer) T(std::move(*ptr));
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        auto ptr = reinterpret_cast<const T*>(&value.buffer);
        ptr->~T();
    }
};

template<typename T>
struct function_table_selector<T, false>
{
    static MetaType_ID type() noexcept
    {
        return metaTypeId<T>();
    }

    static void* access(const variant_type_storage &value) noexcept
    {
        auto ptr = static_cast<const T*>(value.ptr);
        return const_cast<T*>(ptr);
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        auto ptr = static_cast<const T*>(src.ptr);
        dst.ptr = new T(*ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        auto ptr = static_cast<const T*>(value.ptr);
        delete ptr;
    }
};

template<typename T>
inline const variant_function_table* function_table_for()
{
    static const auto result = variant_function_table{
        &function_table_selector<T>::type,
        &function_table_selector<T>::access,
        &function_table_selector<T>::clone,
        &function_table_selector<T>::move,
        &function_table_selector<T>::destroy
    };
    return &result;
}

template<>
inline const variant_function_table* function_table_for<void>()
{
    static const auto result = variant_function_table{
        [] () noexcept -> MetaType_ID { return metaTypeId<void>(); },
        [] (const variant_type_storage&) noexcept -> void* { return nullptr; },
        [] (const variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {}
    };
    return &result;
}

// forward
template<typename T>
void* variant_cast_helper(const variant *value) noexcept;

} // namespace internal

// forward
template<typename T> T& variant_cast(variant&);
template<typename T> const T& variant_cast(const variant&);
template<typename T> T&& variant_cast(variant&&);

class DLL_PUBLIC variant final
{
public:
    variant() noexcept = default;

    variant(const variant &other)
        : manager{other.manager}
    {
        manager->f_clone(other.storage, storage);
    }

    variant& operator=(const variant &other)
    {
        if (this != &other)
            variant{other}.swap(*this);
        return *this;
    }

    variant(variant &&other) noexcept
    {
        swap(other);
    }

    variant& operator=(variant &&other) noexcept
    {
        variant{std::move(other)}.swap(*this);
        return *this;
    }

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<variant, internal::decay_t<T>>::value>
             ::type>
    variant(T &&value)
        : variant{std::forward<T>(value), internal::is_inplace<T>{}}
    {
        static constexpr bool valid = std::is_copy_constructible<T>::value;
        static_assert(valid, "The contained object must be CopyConstructible");
    }

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<variant, internal::decay_t<T>>::value>
             ::type>
    variant& operator=(T &&value)
    {
        variant{std::forward<T>(value)}.swap(*this);
        return *this;
    }

    ~variant() noexcept
    {
        manager->f_destroy(storage);
        manager = internal::function_table_for<void>();
        storage = {.buffer = {0}};
    }

    void clear() noexcept
    {
        variant{}.swap(*this);
    }

    bool empty() const noexcept
    {
        return manager == internal::function_table_for<void>();
    }

    explicit operator bool() const noexcept
    {
        return !empty();
    }

    void swap(variant &other) noexcept
    {
        storage_t temporary = {.buffer = {0}};
        manager->f_move(storage, temporary);
        other.manager->f_move(other.storage, storage);
        manager->f_move(temporary, other.storage);

        std::swap(manager, other.manager);
    }

    MetaType_ID type() const noexcept
    {
        return manager->f_type();
    }

    template<typename T>
    bool is() const noexcept
    {
        return (metaTypeId<T>() == type());
    }

    template<typename T>
    const T& value() const &
    {
        return variant_cast<T>(*this);
    }

    template<typename T>
    T& value() &
    {
        return variant_cast<T>(*this);
    }

    template<typename T>
    T&& value() &&
    {
        return variant_cast<T>(std::move(*this));
    }

    template<typename T>
    T to() const
    {
        static_assert(std::is_move_constructible<T>::value,
                      "Type should be MoveConstructible");

        if (is<T>())
            return value<T>();

        auto fromId = type();
        auto toId = metaTypeId<T>();
        if (MetaType::hasConverter(fromId, toId))
        {
            alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
            if (MetaType::convert(raw_data_ptr(), fromId, &buffer, toId))
                return std::move(*reinterpret_cast<T*>(&buffer));
            throw bad_variant_convert{"Conversion failed"};

        }
        throw bad_variant_convert{"Converter function not found"};
    }

    template<typename T>
    bool as() const
    {
        if (is<T>())
            return true;

        auto fromId = type();
        auto toId = metaTypeId<T>();
        if (MetaType::hasConverter(fromId, toId))
        {
            alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
            if (MetaType::convert(raw_data_ptr(), fromId, &buffer, toId))
            {
                *this = std::move(*reinterpret_cast<T*>(&buffer));
                return true;
            }
            return false;

        }
        return false;
    }

    static const variant empty_variant;
private:
    template<typename T>
    variant(T &&value, std::true_type)
        : manager{internal::function_table_for<internal::decay_t<T>>()}
    {
        new (&storage.buffer) internal::decay_t<T>(std::forward<T>(value));
    }

    template<typename T>
    variant(T &&value, std::false_type)
        : manager{internal::function_table_for<internal::decay_t<T>>()}
    {
        storage.ptr = new internal::decay_t<T>(std::forward<T>(value));
    }

    void* raw_data_ptr() const noexcept
    {
        return manager->f_access(storage);
    }

    using table_t = const internal::variant_function_table;
    using storage_t = internal::variant_type_storage;

    table_t* manager = internal::function_table_for<void>();
    storage_t storage = {.buffer = {0}};

    template<typename T>
    friend void* internal::variant_cast_helper(const variant *value) noexcept;
    friend class std::hash<rtti::variant>;
    friend class rtti::argument;
};

namespace internal {
template<typename T>
inline void* variant_cast_helper(const variant *value) noexcept
{
    return value && value->is<T>() ? value->raw_data_ptr() : nullptr;
}
} //namespace internal

template<typename T>
inline T* variant_cast(variant *value) noexcept
{
    return static_cast<T*>(internal::variant_cast_helper<T>(value));
}

template<typename T>
inline const T* variant_cast(const variant *value)  noexcept
{
    return static_cast<T*>(internal::variant_cast_helper<T>(value));
}

template<typename T>
inline const T& variant_cast(const variant &value)
{
    auto result = variant_cast<internal::decay_t<T>>(&value);
    if (!result)
    {
        if (!value)
            throw bad_variant_cast{"Variant is NULL"};
        else
            throw bad_variant_cast{"Incompatible variant type"};
    }
    return *result;
}

template<typename T>
inline T& variant_cast(variant &value)
{
    auto result = variant_cast<internal::decay_t<T>>(&value);
    if (!result)
    {
        if (!value)
            throw bad_variant_cast{"Variant is NULL"};
        else
            throw bad_variant_cast{"Incompatible variant type"};
    }
    return *result;
}

template<typename T>
inline T&& variant_cast(variant &&value)
{
    auto result = variant_cast<internal::decay_t<T>>(&value);
    if (!result)
    {
        if (!value)
            throw bad_variant_cast{"Variant is NULL"};
        else
            throw bad_variant_cast{"Incompatible variant type"};
    }
    return std::move(*result);
}

} //namespace rtti

namespace std {

inline void swap(rtti::variant &lhs, rtti::variant &rhs) noexcept
{
    lhs.swap(rhs);
}

template<>
struct hash<rtti::variant>: public std::__hash_base<std::size_t, rtti::variant>
{
    using this_t = hash<rtti::variant>;
    typename this_t::result_type operator()(const typename this_t::argument_type &value) const
    {
        if (!value)
            return 0;

        auto type = rtti::MetaType{value.type()};
        return _Hash_impl::hash(value.raw_data_ptr(), type.typeSize());
    }
};

} //namespace std

#endif // VARIANT_H
