#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"

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

template<typename T,
         bool Small = sizeof(typename std::decay<T>::type) <= sizeof(void*),
         bool Safe = std::is_move_constructible<T>::value>
using is_inplace = std::integral_constant<bool, Small && Safe>;

template<typename T>
using is_polymorphic = typename std::conditional<
    std::is_polymorphic<
        typename std::remove_pointer<
            typename std::remove_reference<T>::type
        >::type
    >::value,
    std::true_type, std::false_type>::type;

union DLL_PUBLIC variant_type_storage {
    void *ptr;
    std::aligned_storage<sizeof(void*), sizeof(void*)>::type buffer;
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
        auto ptr = reinterpret_cast<const T*>(&src.buffer);
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

class DLL_PUBLIC bad_variant_cast final: public std::bad_cast
{
public:
    const char* what() const noexcept override
    { return "Bad variant cast"; }
};

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
        storage = {nullptr};
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
        manager->f_move(storage, other.storage);
        std::swap(manager, other.manager);
        //std::swap(storage, other.storage);
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


    static const variant empty_variant;
private:
    template<typename T>
    friend void* internal::variant_cast_helper(const variant *value) noexcept;
    friend class rtti::argument;

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
    storage_t storage = {nullptr};
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
        throw bad_variant_cast{};
    return *result;
}

//template<typename T>
//inline T variant_cast(const variant &value)
//{
//    static constexpr bool valid =
//            std::is_copy_constructible<T>::value ||
//            (std::is_lvalue_reference<T>::value &&
//             std::is_const<typename std::remove_reference<T>::type>::value);
//    static_assert(valid, "Type must be CopyConstructible or Lvalue reference to Const");

//    auto result = variant_cast<internal::decay_t<T>>(&value);
//    if (!result)
//        throw bad_variant_cast{};
//    return *result;
//}

template<typename T>
inline T& variant_cast(variant &value)
{
    auto result = variant_cast<internal::decay_t<T>>(&value);
    if (!result)
        throw bad_variant_cast{};
    return *result;
}

//template<typename T>
//inline T variant_cast(variant &value)
//{
//    static constexpr bool valid =
//            std::is_copy_constructible<T>::value ||
//            std::is_lvalue_reference<T>::value;
//    static_assert(valid, "Type must be CopyConstructible or Lvalue reference");

//    auto result = variant_cast<internal::decay_t<T>>(&value);
//    if (!result)
//        throw bad_variant_cast{};
//    return *result;
//}

template<typename T>
inline T&& variant_cast(variant &&value)
{
    auto result = variant_cast<internal::decay_t<T>>(&value);
    if (!result)
        throw bad_variant_cast{};
    return std::move(*result);
}

} //namespace rtti

namespace std {
inline void swap(rtti::variant &lhs, rtti::variant &rhs) noexcept
{
    lhs.swap(rhs);
}
} //namespace std

#endif // VARIANT_H
