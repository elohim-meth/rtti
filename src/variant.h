#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"
#include "metaclass.h"

#include <finally.h>

#include <cassert>

namespace rtti {

class argument;

namespace internal {

constexpr std::size_t STORAGE_ALIGN = sizeof(void*);
constexpr std::size_t STORAGE_SIZE = sizeof(void*) * 2;

template<typename T, bool Small = sizeof(T) <= STORAGE_SIZE>
using is_inplace = std::integral_constant<bool, Small>;

template<typename T>
using is_inplace_t = typename is_inplace<T>::type;

template<typename T>
constexpr auto is_inplace_v = is_inplace<T>::value;


template<typename T>
struct is_reference_wrapper: std::false_type
{};

template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>>: std::true_type
{};

template<typename T>
using is_reference_wrapper_t = typename is_reference_wrapper<T>::type;

template<typename T>
constexpr auto is_reference_wrapper_v = is_reference_wrapper<T>::value;

template<typename T, bool = is_reference_wrapper_v<T>>
struct unwrap_reference;

template<typename T>
struct unwrap_reference<T, false>: mpl::identity<T>
{};

template<typename T>
struct unwrap_reference<T, true>: mpl::identity<typename T::type>
{};

template<typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

union DLL_PUBLIC variant_type_storage
{
    void *ptr;
    std::aligned_storage_t<STORAGE_SIZE, STORAGE_ALIGN> buffer;
};

enum class type_attribute {NONE, LREF, RREF, LREF_CONST};

struct DLL_PUBLIC variant_function_table
{
    using type_t = MetaType_ID(*)(type_attribute);
    using access_t = void const* (*) (variant_type_storage const&);
    using copy_construct_t = void (*) (void const*, variant_type_storage&);
    using move_construct_t = void (*) (void*, variant_type_storage&);
    using copy_t = void (*) (variant_type_storage const&, variant_type_storage&);
    using move_t = void (*) (variant_type_storage&, variant_type_storage&);
    using destroy_t = void (*) (variant_type_storage&);
    using info_t = ClassInfo (*) (variant_type_storage const&);

    type_t const f_type = nullptr;
    access_t const f_access = nullptr;
    copy_construct_t const f_copy_construct = nullptr;
    move_construct_t const f_move_construct = nullptr;
    copy_t const f_copy = nullptr;
    move_t const f_move = nullptr;
    destroy_t const f_destroy = nullptr;
    info_t const f_info = nullptr;

    variant_function_table(type_t type, access_t access,
                           copy_construct_t copy_construct,
                           move_construct_t move_construct,
                           copy_t copy, move_t move,
                           destroy_t destroy, info_t info) noexcept
        : f_type{type}, f_access{access},
          f_copy_construct{copy_construct},
          f_move_construct{move_construct},
          f_copy{copy}, f_move{move},
          f_destroy{destroy}, f_info(info)
    {}
};

template<typename T,
         bool = is_inplace_v<std::remove_cv_t<T>>,
         bool = is_reference_wrapper_v<std::remove_cv_t<T>>
        >
struct variant_function_table_impl;

template<typename T>
struct variant_function_table_impl<T, true, false>
{
    using Decay = remove_all_cv_t<T>;

    static MetaType_ID type(type_attribute attr)
    {
        if (attr == type_attribute::NONE)
            return metaTypeId<U>();
        else if (attr == type_attribute::LREF)
            return metaTypeId<ULref>();
        else if (attr == type_attribute::RREF)
            return metaTypeId<URref>();
        else if (attr == type_attribute::LREF_CONST)
            return metaTypeId<UConstLref>();
        return metaTypeId<U>();
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return &value.buffer;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        type_manager_t<Decay>::copy_construct(value, &storage.buffer);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        type_manager_t<Decay>::move_or_copy(value, true, &storage.buffer);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        type_manager_t<Decay>::copy_construct(&src.buffer, &dst.buffer);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        type_manager_t<Decay>::move_or_copy(&src.buffer, true, &dst.buffer);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(&value.buffer);
    }
private:
    using U = std::remove_cv_t<T>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};

template<typename T>
struct variant_function_table_impl<T, true, true>
{
    static MetaType_ID type(type_attribute attr)
    {
        if (attr == type_attribute::NONE)
            return metaTypeId<U>();
        else if (attr == type_attribute::LREF)
            return metaTypeId<ULref>();
        else if (attr == type_attribute::RREF)
            return metaTypeId<URref>();
        else if (attr == type_attribute::LREF_CONST)
            return metaTypeId<UConstLref>();
        return metaTypeId<U>();
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        if constexpr(std::is_array_v<U>)
            return &value.ptr;
        else
            return value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage) noexcept
    {
        auto ptr = static_cast<Wrapper const*>(value);
        storage.ptr = const_cast<Decay*>(&ptr->get());
    }

    static void move_construct(void *value, variant_type_storage &storage) noexcept
    {
        auto ptr = static_cast<Wrapper*>(value);
        storage.ptr = const_cast<Decay*>(&ptr->get());
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst) noexcept
    {
        dst.ptr = src.ptr;
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        dst.ptr = src.ptr;
    }

    static void destroy(variant_type_storage&) noexcept
    { /* do nothing */ }
private:
    using Wrapper = remove_all_cv_t<T>;
    using U = unwrap_reference_t<Wrapper>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;

    using Decay = std::conditional_t<std::is_array_v<U>, remove_all_cv_t<U>, full_decay_t<U>>;
};

template<typename T>
struct variant_function_table_impl<T, false, false>
{
    using Decay = remove_all_cv_t<T>;

    static MetaType_ID type(type_attribute attr)
    {
        if (attr == type_attribute::NONE)
            return metaTypeId<U>();
        else if (attr == type_attribute::LREF)
            return metaTypeId<ULref>();
        else if (attr == type_attribute::RREF)
            return metaTypeId<URref>();
        else if (attr == type_attribute::LREF_CONST)
            return metaTypeId<UConstLref>();
        return metaTypeId<U>();
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(value, storage.ptr);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::move_or_copy(value, true, storage.ptr);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        dst.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(src.ptr, dst.ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(value.ptr);
        type_manager_t<Decay>::deallocate(value.ptr);
    }

private:
    using U = std::remove_cv_t<T>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};

template<typename T, std::size_t N>
struct variant_function_table_impl<T[N], false, false>
{
    using Decay = remove_all_cv_t<T[N]>;
    using Base = std::remove_all_extents_t<Decay>;

    static MetaType_ID type(type_attribute attr)
    {
        if (attr == type_attribute::NONE)
            return metaTypeId<U>();
        else if (attr == type_attribute::LREF)
            return metaTypeId<ULref>();
        else if (attr == type_attribute::RREF)
            return metaTypeId<URref>();
        else if (attr == type_attribute::LREF_CONST)
            return metaTypeId<UConstLref>();
        return metaTypeId<U>();
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return &value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Base>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(value, storage.ptr);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Base>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::move_or_copy(value, true, storage.ptr);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Base>)
    {
        dst.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(src.ptr, dst.ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(value.ptr);
        type_manager_t<Decay>::deallocate(value.ptr);
    }

private:
    using U = Decay;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};


// Since two pointer indirections when casting array<T> -> T*
// we need to use temp storage and have no room for inplace
template<typename T, std::size_t N>
struct variant_function_table_impl<T[N], true, false>:
       variant_function_table_impl<T[N], false, false>
{};

template<typename T>
struct class_info_get
{
    static ClassInfo info(variant_type_storage const &value)
    {
        if constexpr(!(std::is_class_v<Decay> || is_class_ptr_v<Decay>))
        {
            return ClassInfo{};
        }
        else
        {
            using is_registered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
            auto instance = Selector::access(value);
            if constexpr(std::is_class_v<Decay>)
            {
                if constexpr(is_registered::value)
                {
                    auto ptr = static_cast<C const*>(instance);
                    return ptr->classInfo();
                }
                else
                    return ClassInfo{metaTypeId<C>(), instance};
            }
            else if constexpr(is_class_ptr_v<Decay>)
            {
                if constexpr(is_registered::value)
                {
                    auto ptr = reinterpret_cast<C const * const *>(instance);
                    return (*ptr)->classInfo();
                }
                else
                {
                    auto ptr = reinterpret_cast<C const * const *>(instance);
                    return ClassInfo{metaTypeId<C>(), *ptr};
                }
            }
        }
    }
private:
    using Unwrap = unwrap_reference_t<std::remove_cv_t<T>>;
    using Decay = std::conditional_t<std::is_array_v<Unwrap>, void, full_decay_t<Unwrap>>;
    using Selector = variant_function_table_impl<T>;
    using C = std::remove_pointer_t<Decay>;
};

template<typename T>
inline variant_function_table const* variant_function_table_for() noexcept
{
    static auto const result = variant_function_table{
        &variant_function_table_impl<T>::type,
        &variant_function_table_impl<T>::access,
        &variant_function_table_impl<T>::copy_construct,
        &variant_function_table_impl<T>::move_construct,
        &variant_function_table_impl<T>::copy,
        &variant_function_table_impl<T>::move,
        &variant_function_table_impl<T>::destroy,
        &class_info_get<T>::info
    };
    return &result;
}

template<>
inline variant_function_table const* variant_function_table_for<void>() noexcept
{
    static auto const result = variant_function_table{
        [] (type_attribute) noexcept -> MetaType_ID { return MetaType_ID(); },
        [] (variant_type_storage const&) noexcept -> void const* { return nullptr; },
        [] (void const*, variant_type_storage&) noexcept {},
        [] (void*, variant_type_storage&) noexcept {},
        [] (variant_type_storage const&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {},
        [] (variant_type_storage const&) noexcept { return ClassInfo(); }
    };
    return &result;
}

} // namespace internal

class DLL_PUBLIC variant final
{
public:
    variant() = default;

    variant(variant const &other);
    variant& operator=(variant const &other);
    variant(variant &&other) noexcept;
    variant& operator=(variant &&other) noexcept;

    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant(T &&value)
        : manager{internal::variant_function_table_for<std::remove_reference_t<T>>()}
    {
        using NoRef = std::remove_reference_t<T>;
        using Type = std::conditional_t<std::is_array_v<NoRef>, std::remove_all_extents_t<NoRef>, NoRef>;
        constexpr auto move = !std::is_reference_v<T> && !std::is_const_v<T>;
        constexpr auto valid = std::is_copy_constructible_v<Type>
            || (move && std::is_move_constructible_v<Type>);
        static_assert(valid, "The contained type must be CopyConstructible or MoveConstructible");
        using selector_t = std::conditional_t<move, std::true_type, std::false_type>;

        constructor(std::addressof(value), selector_t{});
    }

    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant& operator=(T &&value)
    {
        variant{std::forward<T>(value)}.swap(*this);
        return *this;
    }

    ~variant() noexcept;

    void clear() noexcept;
    bool empty() const noexcept;
    explicit operator bool() const noexcept
    { return !empty(); }

    MetaType_ID typeId() const noexcept
    { return internalTypeId(); }
    MetaClass const* metaClass() const
    {
        auto const &info = classInfo();
        return MetaClass::find(info.typeId);
    }

    template<typename T>
    bool is()
    {
        auto typeId = internalTypeId(type_attribute::LREF);
        return metafunc_is<T>::invoke(*this, typeId);
    }

    template<typename T>
    bool is() const
    {
        auto typeId = internalTypeId(type_attribute::LREF_CONST);
        return metafunc_is<T>::invoke(*this, typeId);
    }

    template<typename T>
    T& value() &
    {
        using U = std::remove_reference_t<T>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& value() const &
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T&& value() &&
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::NONE);
        auto toId = metaTypeId<U>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return std::move(*result);
    }

    template<typename T>
    T const& cvalue()
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& cvalue() const
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T* data()
    {
        using U = std::remove_reference_t<T>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        try {
            return metafunc_cast<U>::invoke(*this, fromId, toId);
        } catch (...) {
            return nullptr;
        }
    }

    template<typename T>
    T const* data() const
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        try {
            return metafunc_cast<U>::invoke(*this, fromId, toId);
        } catch (...) {
            return nullptr;
        }
    }

    template<typename T>
    T to()
    {
        static_assert(!std::is_reference_v<T>, "Type cannot be reference");

        std::aligned_storage_t<sizeof(T), alignof(T)> buffer;
        auto typeId = internalTypeId(type_attribute::LREF);
        metafunc_to<T>::invoke(*this, typeId, &buffer);
        FINALLY { type_manager_t<T>::destroy(&buffer); };
        return internal::move_or_copy<T>(&buffer, true);
    }

    template<typename T>
    T to() const
    {
        static_assert(!std::is_reference_v<T>, "Type cannot be reference");

        std::aligned_storage_t<sizeof(T), alignof(T)> buffer;
        auto typeId = internalTypeId(type_attribute::LREF_CONST);
        metafunc_to<T>::invoke(*this, typeId, &buffer);
        FINALLY { type_manager_t<T>::destroy(&buffer); };
        return internal::move_or_copy<T>(&buffer, true);
    }

    template<typename T>
    bool canConvert()
    {
        static_assert(!std::is_reference_v<T>, "Type cannot be reference");

        return is<T>() ||
               MetaType::hasConverter(
                    internalTypeId(type_attribute::LREF),
                    metaTypeId<T>());
    }

    template<typename T>
    bool canConvert() const
    {
        static_assert(!std::is_reference_v<T>, "Type cannot be reference");

        return is<T>() ||
               MetaType::hasConverter(
                    internalTypeId(type_attribute::LREF_CONST),
                    metaTypeId<T>());
    }

    template<typename T>
    void convert()
    { *this = to<T>(); }

    template<typename T>
    bool tryConvert()
    {
        try
        {
            *this = to<T>();
            return true;
        }
        catch (...)
        {
            return false;
        };
    }

    using type_attribute = internal::type_attribute;
    static variant const empty_variant;
private:
    void swap(variant &other) noexcept;

    void const* raw_data_ptr() const noexcept
    { return manager->f_access(storage); }
    void * raw_data_ptr() noexcept
    { return const_cast<void*>(manager->f_access(storage)); }
    MetaType_ID internalTypeId(type_attribute attr = type_attribute::NONE) const noexcept
    { return manager->f_type(attr); }
    ClassInfo classInfo() const noexcept
    { return manager->f_info(storage); }

    void constructor(void *value, std::true_type)
    { manager->f_move_construct(value, storage); }
    void constructor(void const *value, std::false_type)
    { manager->f_copy_construct(value, storage); }

    template<typename T>
    struct metafunc_is
    {
        static bool invoke(variant const &self, MetaType_ID typeId)
        {
            if (self.empty())
                return false;

            auto from = MetaType{typeId};
            auto to = MetaType{metaTypeId<T>()};

            return MetaType::compatible(from, to) &&
                ((from.decayId() == to.decayId()) || cast(self, from, tag_t{}));
        }

    private:
        using Decay = full_decay_t<T>;
        using C = std::remove_pointer_t<Decay>;
        using tag_t =
            std::conditional_t<std::is_class_v<Decay>, std::integral_constant<int, 1>,
            std::conditional_t<is_class_ptr_v<Decay>,  std::integral_constant<int, 2>,
                                                       std::integral_constant<int, 0>
            >>;

        // nope
        static bool cast(variant const&, MetaType,
                         std::integral_constant<int, 0>)
        { return false; }
        // class
        static bool cast(variant const &self, MetaType from,
                         std::integral_constant<int, 1>)
        {
            if (from.isClass())
                return cast_imp(self);
            return false;
        }
        // class ptr
        static bool cast(variant const &self, MetaType from,
                         std::integral_constant<int, 2>)
        {
            if (from.isClassPtr())
                return cast_imp(self);
            return false;
        }
        // implementaion
        static bool cast_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return false;

            auto fromClass = MetaClass::find(info.typeId);
            auto toClass = MetaClass::find(metaTypeId<C>());
            if (!fromClass || !toClass)
                return false;
            return fromClass->inheritedFrom(toClass);
        }
    };

    template<typename T>
    struct metafunc_cast
    {
        static T* invoke(variant const &self, MetaType_ID fromId, MetaType_ID toId)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};

            Decay const *result = nullptr;
            auto from = MetaType{fromId};
            auto to = MetaType{toId};
            if (MetaType::compatible(from, to))
            {
                if (from.decayId() == to.decayId())
                    result = static_cast<Decay const*>(self.raw_data_ptr());
                else
                {
                    auto ptr = cast(self, from, tag_t{});
                    if (ptr)
                        result = static_cast<Decay const*>(ptr);
                }
            }

            if (!result)
                throw bad_variant_cast{std::string{"Incompatible types: "} +
                                       from.typeName() + " -> " + to.typeName()};
            if constexpr(std::is_array_v<T>)
                return reinterpret_cast<T*>(*result);
            else
                return const_cast<T*>(result);
        }

    private:
        using Decay = full_decay_t<T>;
        using C = std::remove_pointer_t<Decay>;
        using tag_t =
            std::conditional_t<std::is_class_v<Decay>, std::integral_constant<int, 1>,
            std::conditional_t<is_class_ptr_v<Decay>,  std::integral_constant<int, 2>,
                                                       std::integral_constant<int, 0>
            >>;

        // nope
        static void const* cast(variant const&, MetaType,
                                std::integral_constant<int, 0>)
        { return nullptr; }
        // class
        static void const* cast(variant const &self, MetaType from,
                                std::integral_constant<int, 1>)
        {
            if (from.isClass())
                return cast_imp(self);

            return nullptr;
        }
        // class ptr
        static void const* cast(variant const &self, MetaType from,
                                std::integral_constant<int, 2>)
        {
            if (from.isClassPtr())
            {
                auto ptr = cast_imp(self);
                if (ptr)
                {
                    if (ptr == self.storage.ptr)
                        return &self.storage.ptr;
                    else
                        throw bad_variant_cast("Reference to sub-object pointers isn't supported");
                }
            }
            return nullptr;
        }
        // implementaion
        static void const* cast_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return nullptr;

            auto fromClass = MetaClass::find(info.typeId);
            auto toClass = MetaClass::find(metaTypeId<C>());
            if (!fromClass || !toClass)
                return nullptr;

            return fromClass->cast(toClass, info.instance, {});
        }
    };

    template<typename T>
    struct metafunc_to
    {
        static_assert(!std::is_array_v<T>, "Array types aren't supported");

        static void invoke(variant const &self, MetaType_ID typeId, void *buffer)
        {
            assert(buffer);
            if (self.empty())
                throw bad_variant_convert{"Variant is empty"};

            auto from = MetaType{typeId};
            auto to = MetaType{metaTypeId<T>()};
            if (MetaType::compatible(from, to))
            {
                if (from.decayId() == to.decayId())
                {
                    to.copy_construct(self.raw_data_ptr(), buffer);
                    return;
                }
                else if (cast(self, from, to, buffer, tag_t{}))
                    return;
            }

            if (MetaType::hasConverter(from, to))
            {
                if (MetaType::convert(self.raw_data_ptr(), from, buffer, to))
                    return;

                throw bad_variant_convert{std::string{"Conversion failed: "} +
                                          from.typeName() + " -> " + to.typeName()};
            }
            throw bad_variant_convert{std::string{"Incompatible types: "} +
                                      from.typeName() + " -> " + to.typeName()};
        }

    private:
        using Decay = full_decay_t<T>;
        using C = std::remove_pointer_t<Decay>;
        using tag_t =
            std::conditional_t<std::is_class_v<Decay>, std::integral_constant<int, 1>,
            std::conditional_t<is_class_ptr_v<Decay>,  std::integral_constant<int, 2>,
                                                       std::integral_constant<int, 0>
            >>;

        // nope
        static bool cast(variant const&, MetaType, MetaType, void*,
                         std::integral_constant<int, 0>)
        { return false; }
        // class
        static bool cast(variant const &self, MetaType from, MetaType to, void *buffer,
                         std::integral_constant<int, 1>)
        {
            if (from.isClass())
            {
                auto ptr = cast_imp(self);
                if (ptr)
                {
                    to.copy_construct(ptr, buffer);
                    return true;
                }
                return false;
            }
            return false;
        }
        // class ptr
        static bool cast(variant const &self, MetaType from, MetaType to, void *buffer,
                         std::integral_constant<int, 2>)
        {
            if (from.isClassPtr())
            {
                auto ptr = cast_imp(self);
                if (ptr)
                {
                    to.copy_construct(&ptr, buffer);
                    return true;
                }
                return false;
            }
            return false;
        }
        // implementaion
        static void const* cast_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return nullptr;

            auto fromClass = MetaClass::find(info.typeId);
            auto toClass = MetaClass::find(metaTypeId<C>());
            if (!fromClass || !toClass)
                return nullptr;

            return fromClass->cast(toClass, info.instance, {});
        }
    };

    using table_t = internal::variant_function_table const;
    using storage_t = internal::variant_type_storage;

    storage_t storage;
    table_t* manager = internal::variant_function_table_for<void>();

private:
    DECLARE_ACCESS_KEY(TypeIdAccessKey)
        friend class rtti::argument;
    };
    DECLARE_ACCESS_KEY(InternalIsAccessKey)
        friend class rtti::argument;
    };
    DECLARE_ACCESS_KEY(RawPtrAccessKey)
        friend class rtti::argument;
        friend struct std::hash<rtti::variant>;
    };
    DECLARE_ACCESS_KEY(SwapAccessKey)
        friend void swap(variant&, variant&) noexcept;
    };
public:
    MetaType_ID internalTypeId(type_attribute attr, TypeIdAccessKey) const noexcept
    { return internalTypeId(attr); }
    template<typename T>
    static bool internalIs(variant const &v, MetaType_ID typeId, InternalIsAccessKey)
    { return metafunc_is<T>::invoke(v, typeId); }
    void const* raw_data_ptr(RawPtrAccessKey) const noexcept
    { return raw_data_ptr(); }
    void * raw_data_ptr(RawPtrAccessKey) noexcept
    { return raw_data_ptr(); }
    void swap(variant &other, SwapAccessKey) noexcept
    { swap(other); }
};

inline void swap(variant &lhs, variant &rhs) noexcept
{
    lhs.swap(rhs, {});
}

} //namespace rtti

namespace std {

template<>
struct hash<rtti::variant>: public std::__hash_base<std::size_t, rtti::variant>
{
    std::size_t operator()(rtti::variant const &value) const noexcept
    {
        if (!value)
            return 0;

        auto type = rtti::MetaType{value.typeId()};
        auto ptr = value.raw_data_ptr({});
        if (type.isArray())
            ptr = *reinterpret_cast<void const * const *>(ptr);
        return _Hash_impl::hash(ptr, type.typeSize());
    }
};

} //namespace std

#endif // VARIANT_H
