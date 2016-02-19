#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"
#include "metaclass.h"

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
struct is_reference_wrapper: std::false_type
{};

template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>>: std::true_type
{};

template<typename T>
using is_reference_wrapper_t = typename is_reference_wrapper<T>::type;

template<typename T, bool = is_reference_wrapper<T>::value>
struct unwrap_reference;

template<typename T>
struct unwrap_reference<T, false>: identity<T>
{};

template<typename T>
struct unwrap_reference<T, true>: identity<typename T::type>
{};

template<typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

union DLL_PUBLIC variant_type_storage
{
    void *ptr;
    alignas(STORAGE_ALIGN) std::uint8_t buffer[STORAGE_SIZE];

    variant_type_storage(): buffer{0} {}
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
    using info_t = ClassInfo(*)(variant_type_storage const&);

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
         bool = is_inplace<remove_cv_t<T>>::value,
         bool = is_reference_wrapper<remove_cv_t<T>>::value>
struct function_table_selector;

template<typename T>
struct function_table_selector<T, true, false>
{
    using Decay = full_decay_t<T>;

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
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = static_cast<Decay const*>(value);
        copy_selector(ptr, storage, CanCopy{});
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible<Decay>::value)
    {
        auto ptr = static_cast<Decay*>(value);
        move_selector(ptr, storage, CanMove{});
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = reinterpret_cast<Decay const*>(&src.buffer);
        copy_selector(ptr, dst, CanCopy{});
    }

    static void move(variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_move_constructible<Decay>::value)
    {
        auto ptr = reinterpret_cast<Decay*>(&src.buffer);
        move_selector(ptr, dst, CanMove{});
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Decay>::value)
    {
        auto ptr = reinterpret_cast<Decay const*>(&value.buffer);
        ptr->~Decay();
    }
private:
    using U = remove_cv_t<T>;
    using ULref = add_lvalue_reference_t<U>;
    using URref = add_rvalue_reference_t<U>;
    using UConstLref = add_lvalue_reference_t<add_const_t<U>>;

    using CanCopy = typename std::is_copy_constructible<Decay>::type;
    using CanMove = typename std::is_move_constructible<Decay>::type;

    static void copy_selector(Decay const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move only type");
    }

    static void copy_selector(Decay const *src, variant_type_storage &storage, std::true_type)
    {
        new (&storage.buffer) Decay(*src);
    }

    static void move_selector(Decay *src, variant_type_storage &storage, std::false_type)
    {
        //degenerate to copy
        copy_selector(src, storage, std::true_type{});
    }

    static void move_selector(Decay *src, variant_type_storage &storage, std::true_type)
    {
        new (&storage.buffer) Decay(std::move(*src));
    }
};

template<typename T>
struct function_table_selector<T, true, true>
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
        return access_selector(value, IsArray{});
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
    {
        // do nothing
    }
private:
    using Wrapper = remove_all_cv_t<T>;
    using U = unwrap_reference_t<Wrapper>;
    using ULref = add_lvalue_reference_t<U>;
    using URref = add_rvalue_reference_t<U>;
    using UConstLref = add_lvalue_reference_t<add_const_t<U>>;

    using IsArray = is_array_t<U>;
    using Decay = conditional_t<IsArray::value, remove_all_cv_t<U>, full_decay_t<U>>;

    static void const* access_selector(variant_type_storage const &value, std::false_type) noexcept
    {
        return value.ptr;
    }

    static void const* access_selector(variant_type_storage const &value, std::true_type) noexcept
    {
        return &value.ptr;
    }
};

template<typename T>
struct function_table_selector<T, false, false>
{
    using Decay = full_decay_t<T>;

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
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = static_cast<Decay const*>(value);
        copy_selector(ptr, storage, CanCopy{});
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible<Decay>::value)
    {
        auto ptr = static_cast<Decay*>(value);
        move_selector(ptr, storage, CanMove{});
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = static_cast<Decay const*>(src.ptr);
        copy_selector(ptr, dst, CanCopy{});
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Decay>::value)
    {
        auto ptr = static_cast<Decay const*>(value.ptr);
        delete ptr;
    }

private:
    using U = remove_cv_t<T>;
    using ULref = add_lvalue_reference_t<U>;
    using URref = add_rvalue_reference_t<U>;
    using UConstLref = add_lvalue_reference_t<add_const_t<U>>;

    using CanCopy = typename std::is_copy_constructible<Decay>::type;
    using CanMove = typename std::is_move_constructible<Decay>::type;

    static void copy_selector(Decay const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move-only type");
    }

    static void copy_selector(Decay const *src, variant_type_storage &storage, std::true_type)
    {
        storage.ptr = new Decay(*src);
    }

    static void move_selector(Decay *src, variant_type_storage &storage, std::false_type)
    {
        //degenerate to copy
        copy_selector(src, storage, std::true_type{});
    }

    static void move_selector(Decay *src, variant_type_storage &storage, std::true_type)
    {
        storage.ptr = new Decay(std::move(*src));
    }
};

template<typename T, std::size_t N>
struct function_table_selector<T[N], false, false>
{
    using Decay = remove_all_cv_t<T[N]>;
    using Base = remove_all_extents_t<Decay>;
    using Allocator = std::allocator<Base>;

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
        noexcept(std::is_nothrow_copy_constructible<Base>::value)
    {
        auto from = static_cast<Base const*>(value);
        copy_selector(from, storage, CanCopy{});
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible<Base>::value)
    {
        auto from = static_cast<Base*>(value);
        move_selector(from, storage, CanMove{});
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Base>::value)
    {
        auto from = static_cast<Base const*>(src.ptr);
        copy_selector(from, dst, CanCopy{});
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Base>::value)
    {
        constexpr auto length = array_length<T[N]>::value;

        auto alloc = Allocator{};
        auto ptr = static_cast<Base*>(value.ptr);
        std::_Destroy(ptr, ptr + length);
        alloc.deallocate(ptr, length);
    }

private:
    using U = Decay;
    using ULref = add_lvalue_reference_t<U>;
    using URref = add_rvalue_reference_t<U>;
    using UConstLref = add_lvalue_reference_t<add_const_t<U>>;

    using CanCopy = typename std::is_copy_constructible<Base>::type;
    using CanMove = typename std::is_copy_constructible<Base>::type;

    static void copy_selector(Base const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move-only type");
    }

    static void copy_selector(Base const *src, variant_type_storage &storage, std::true_type)
    {
        constexpr auto length = array_length<T[N]>::value;

        auto alloc = Allocator{};
        auto to = alloc.allocate(length);
        std::copy(src, src + length, to);
        storage.ptr = to;
    }

    static void move_selector(Base const *src, variant_type_storage &storage, std::false_type)
    {
        //degenerate to copy
        copy_selector(src, storage, std::true_type{});
    }

    static void move_selector(Base const *src, variant_type_storage &storage, std::true_type)
    {
        constexpr auto length = array_length<T[N]>::value;

        auto alloc = Allocator{};
        auto to = alloc.allocate(length);
        std::move(src, src + length, to);
        storage.ptr = to;
    }
};


// Since two pointer indirections when casting array<T> -> T*
// we need to use temp storage and have no room for inplace
template<typename T, std::size_t N>
struct function_table_selector<T[N], true, false>:
       function_table_selector<T[N], false, false>
{};

template<typename T>
struct class_info_get
{
    static ClassInfo info(variant_type_storage const &value)
    {
        return info_selector(value, IsClass{}, IsClassPtr{});
    }
private:
    using Unwrap = unwrap_reference_t<remove_cv_t<T>>;
    using Decay = conditional_t<is_array_t<Unwrap>::value, void, full_decay_t<Unwrap>>;
    using Selector = function_table_selector<T>;
    using IsClass = is_class_t<Decay>;
    using IsClassPtr = is_class_ptr_t<Decay>;
    using C = remove_pointer_t<Decay>;

    static ClassInfo info_selector(variant_type_storage const&, std::false_type, std::false_type)
    {
        return ClassInfo{};
    }

    static ClassInfo info_selector(variant_type_storage const &value, std::true_type, std::false_type)
    {
        using IsRegistered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
        return info_selector_registered(value, IsRegistered{});
    }
    static ClassInfo info_selector_registered(variant_type_storage const &value, std::false_type)
    {
        return ClassInfo{metaTypeId<C>(), Selector::access(value)};
    }
    static ClassInfo info_selector_registered(variant_type_storage const &value, std::true_type)
    {
        auto ptr = static_cast<C const*>(Selector::access(value));
        return ptr->classInfo();
    }

    static ClassInfo info_selector(variant_type_storage const &value, std::false_type, std::true_type)
    {
        using IsRegistered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
        return info_selector_registered_ptr(value, IsRegistered{});
    }
    static ClassInfo info_selector_registered_ptr(variant_type_storage const &value, std::false_type)
    {
        auto ptr = reinterpret_cast<C const * const *>(Selector::access(value));
        return ClassInfo{metaTypeId<C>(), *ptr};
    }
    static ClassInfo info_selector_registered_ptr(variant_type_storage const &value, std::true_type)
    {
        auto ptr = reinterpret_cast<C const * const *>(Selector::access(value));
        return (*ptr)->classInfo();
    }
};

template<typename T>
inline variant_function_table const* function_table_for() noexcept
{
    static auto const result = variant_function_table{
        &function_table_selector<T>::type,
        &function_table_selector<T>::access,
        &function_table_selector<T>::copy_construct,
        &function_table_selector<T>::move_construct,
        &function_table_selector<T>::copy,
        &function_table_selector<T>::move,
        &function_table_selector<T>::destroy,
        &class_info_get<T>::info
    };
    return &result;
}

template<>
inline variant_function_table const* function_table_for<void>() noexcept
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
             typename = enable_if_t<!std::is_same<decay_t<T>, variant>::value>>
    variant(T &&value)
        : manager{internal::function_table_for<remove_reference_t<T>>()}
    {
        using NoRef = remove_reference_t<T>;
        using Type = conditional_t<std::is_array<NoRef>::value, remove_all_extents_t<NoRef>, NoRef>;
        constexpr bool move = !std::is_reference<T>::value && !std::is_const<T>::value;
        constexpr bool valid = std::is_copy_constructible<Type>::value
            || (move && std::is_move_constructible<Type>::value);
        static_assert(valid, "The contained type must be CopyConstructible or MoveConstructible");
        using selector_t = conditional_t<move, std::true_type, std::false_type>;

        constructor_selector(std::addressof(value), selector_t{});
    }

    template<typename T,
             typename = enable_if_t<!std::is_same<decay_t<T>, variant>::value>>
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

    MetaType_ID typeId() const
    { return internalTypeId(); }
    MetaClass const* metaClass() const
    {
        auto const &info = classInfo();
        return MetaClass::findByTypeId(info.typeId);
    }


    template<typename T>
    bool is() const
    {
        auto typeId = internalTypeId(type_attribute::LREF_CONST);
        return metafunc_is<T>::invoke(*this, typeId);
    }

    template<typename T>
    bool is()
    {
        auto typeId = internalTypeId(type_attribute::LREF);
        return metafunc_is<T>::invoke(*this, typeId);
    }

    template<typename T>
    T const& value() const &
    {
        using U = add_const_t<remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T& value() &
    {
        using U = remove_reference_t<T>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<add_lvalue_reference_t<U>>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& cvalue()
    {
        using U = add_const_t<remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& cvalue() const
    {
        using U = add_const_t<remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T&& value() &&
    {
        using U = remove_cv_t<remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::NONE);
        auto toId = metaTypeId<U>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return std::move(*result);
    }

    template<typename T>
    T to()
    {
        static_assert(!std::is_reference<T>::value,
                      "Type cannot be reference");
        static_assert(std::is_move_constructible<T>::value,
                      "Type should be MoveConstructible");

        alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
        auto typeId = internalTypeId(type_attribute::LREF);
        metafunc_to<T>::invoke(*this, typeId, &buffer);
        return std::move(*reinterpret_cast<T*>(&buffer));
    }

    template<typename T>
    T to() const
    {
        static_assert(!std::is_reference<T>::value,
                      "Type cannot be reference");
        static_assert(std::is_move_constructible<T>::value,
                      "Type should be MoveConstructible");

        alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
        auto typeId = internalTypeId(type_attribute::LREF_CONST);
        metafunc_to<T>::invoke(*this, typeId, &buffer);
        return std::move(*reinterpret_cast<T*>(&buffer));
    }

    template<typename T>
    void convert()
    { *this = to<T>(); }

    static variant const empty_variant;
private:
    using type_attribute = internal::type_attribute;

    void swap(variant &other) noexcept;

    void const* raw_data_ptr() const
    { return manager->f_access(storage); }
    void * raw_data_ptr()
    { return const_cast<void*>(manager->f_access(storage)); }
    MetaType_ID internalTypeId(type_attribute attr = type_attribute::NONE) const
    { return manager->f_type(attr); }
    ClassInfo classInfo() const
    { return manager->f_info(storage); }

    void constructor_selector(void *value, std::true_type)
    { manager->f_move_construct(value, storage); }
    void constructor_selector(void const *value, std::false_type)
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

            if (from.decayId() == to.decayId())
                return MetaType::compatible(from, to);

            return cast_selector(self, from, to, IsClass{}, IsClassPtr{});
        }

    private:
        using Decay = full_decay_t<T>;
        using IsClass = is_class_t<Decay>;
        using IsClassPtr = is_class_ptr_t<Decay>;
        using C = remove_pointer_t<Decay>;

        // nope
        static bool cast_selector(variant const&, MetaType, MetaType,
                                  std::false_type, std::false_type)
        { return false; }
        // class
        static bool cast_selector(variant const &self, MetaType from, MetaType to,
                                  std::true_type, std::false_type)
        {
            if (from.isClass() && MetaType::compatible(from, to))
                return invoke_imp(self);
            return false;
        }
        // class ptr
        static bool cast_selector(variant const &self, MetaType from, MetaType to,
                                  std::false_type, std::true_type)
        {
            if (from.isClassPtr() && MetaType::compatible(from, to))
                return invoke_imp(self);
            return false;
        }
        // implementaion
        static bool invoke_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return false;

            auto fromClass = MetaClass::findByTypeId(info.typeId);
            auto toClass = MetaClass::findByTypeId(metaTypeId<C>());
            if (!fromClass && !toClass)
                return false;
            return fromClass->inheritedFrom(toClass);
        }
    };

    template<typename T>
    struct metafunc_cast
    {
        using Decay = full_decay_t<T>;

        static T* invoke(variant const &self, MetaType_ID fromId, MetaType_ID toId)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};

            Decay const *result = nullptr;
            auto from = MetaType{fromId};
            auto to = MetaType{toId};
            if (from.decayId() == to.decayId())
            {
                if (MetaType::compatible(from, to))
                    result = static_cast<Decay const*>(self.raw_data_ptr());
            }
            else
            {
               auto ptr = cast_selector(self, from, to, IsClass{}, IsClassPtr{});
               if (ptr)
                   result = static_cast<Decay const*>(ptr);
            }

            if (!result)
                throw bad_variant_cast{std::string{"Incompatible types: "} +
                                       from.typeName() + " -> " + to.typeName()};
            return result_selector(result, IsArray{});
        }

    private:
        using IsArray = is_array_t<T>;
        using IsClass = is_class_t<Decay>;
        using IsClassPtr = is_class_ptr_t<Decay>;
        using C = remove_pointer_t<Decay>;

        static T* result_selector(Decay const *value, std::false_type)
        { return const_cast<T*>(value); }
        static T* result_selector(Decay const *value, std::true_type)
        { return reinterpret_cast<T*>(*value); }

        // nope
        static void const* cast_selector(variant const&, MetaType, MetaType,
                                     std::false_type, std::false_type)
        { return nullptr; }
        // class
        static void const* cast_selector(variant const &self, MetaType from, MetaType to,
                                     std::true_type, std::false_type)
        {
            if (from.isClass() && MetaType::compatible(from, to))
                return invoke_imp(self);

            return nullptr;
        }
        // class ptr
        static void const* cast_selector(variant const &self, MetaType from, MetaType to,
                                         std::false_type, std::true_type)
        {
            if (from.isClassPtr() && MetaType::compatible(from, to))
            {
                auto ptr = invoke_imp(self);
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
        static void const* invoke_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return nullptr;

            auto fromClass = MetaClass::findByTypeId(info.typeId);
            auto toClass = MetaClass::findByTypeId(metaTypeId<C>());
            if (!fromClass && !toClass)
                return nullptr;

            return fromClass->cast(toClass, info.instance);
        }
    };

    template<typename T>
    struct metafunc_to
    {
        static_assert(!std::is_array<T>::value, "Array types aren't supported");

        using Decay = full_decay_t<T>;

        static void invoke(variant const &self, MetaType_ID typeId, void *buffer)
        {
            assert(buffer);
            if (self.empty())
                throw bad_variant_convert{"Variant is empty"};

            auto from = MetaType{typeId};
            auto to = MetaType{metaTypeId<T>()};
            if (from.decayId() == to.decayId())
            {
                if (MetaType::compatible(from, to))
                {
                    Decay const *value = static_cast<Decay const *>(self.raw_data_ptr());
                    new (buffer) Decay(*value);
                    return;
                }
                else
                    throw bad_variant_convert{std::string{"Incompatible types: "} +
                                           from.typeName() + " -> " + to.typeName()};
            }
            else
            {
                if (cast_selector(self, from, to, buffer, IsClass{}, IsClassPtr{}))
                    return;

                if (MetaType::hasConverter(from, to))
                {
                    if (MetaType::convert(self.raw_data_ptr(), from, buffer, to))
                        return;

                    throw bad_variant_convert{std::string{"Conversion failed: "} +
                                              from.typeName() + " -> " + to.typeName()};
                }
                throw bad_variant_convert{std::string{"Converter not found: "} +
                                          from.typeName() + " -> " + to.typeName()};
            }
        }

    private:
        using IsClass = is_class_t<Decay>;
        using IsClassPtr = is_class_ptr_t<Decay>;
        using C = remove_pointer_t<Decay>;

        // nope
        static bool cast_selector(variant const&, MetaType, MetaType, void *,
                                  std::false_type, std::false_type)
        { return false; }
        // class
        static bool cast_selector(variant const &self, MetaType from, MetaType, void *buffer,
                                  std::true_type, std::false_type)
        {
            if (from.isClass())
            {
                auto ptr = invoke_imp(self);
                if (ptr)
                {
                    new (buffer) C(*static_cast<C const*>(ptr));
                    return true;
                }
                return false;
            }
            return false;
        }
        // class ptr
        static bool cast_selector(variant const &self, MetaType from, MetaType to, void *buffer,
                                     std::false_type, std::true_type)
        {
            if (from.isClassPtr() && MetaType::compatible(from, to))
            {
                auto ptr = invoke_imp(self);
                if (ptr)
                {
                    new (buffer) T(static_cast<C*>(const_cast<void*>(ptr)));
                    return true;
                }
                return false;
            }
            return false;
        }
        // implementaion
        static void const* invoke_imp(variant const &self)
        {
            auto const &info = self.classInfo();

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return nullptr;

            auto fromClass = MetaClass::findByTypeId(info.typeId);
            auto toClass = MetaClass::findByTypeId(metaTypeId<C>());
            if (!fromClass && !toClass)
                return nullptr;

            return fromClass->cast(toClass, info.instance);
        }
    };

    using table_t = internal::variant_function_table const;
    using storage_t = internal::variant_type_storage;

    storage_t storage;
    table_t* manager = internal::function_table_for<void>();

    friend void swap(variant&, variant&) noexcept;
    friend struct std::hash<rtti::variant>;
    friend class rtti::argument;
};

inline void swap(variant &lhs, variant &rhs) noexcept
{
    lhs.swap(rhs);
}

} //namespace rtti

namespace std {

template<>
struct hash<rtti::variant>: public std::__hash_base<std::size_t, rtti::variant>
{
    using this_t = hash<rtti::variant>;
    typename this_t::result_type operator()(typename this_t::argument_type const &value) const
    {
        if (!value)
            return 0;

        auto type = rtti::MetaType{value.typeId()};
        auto ptr = value.raw_data_ptr();
        if (type.isArray())
            ptr = *reinterpret_cast<void const * const *>(ptr);
        return _Hash_impl::hash(ptr, type.typeSize());
    }
};

} //namespace std

#endif // VARIANT_H
