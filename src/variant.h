#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"
#include "metaclass.h"

#include <cassert>

namespace rtti {

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

struct DLL_PUBLIC variant_function_table
{
    using type_t = MetaType_ID(*)();
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

    static MetaType_ID type()
    {
        return metaTypeId<remove_cv_t<T>>();
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
        new (&storage.buffer) Decay(std::move(*ptr));
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
        new (&dst.buffer) Decay(std::move(*ptr));
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Decay>::value)
    {
        auto ptr = reinterpret_cast<Decay const*>(&value.buffer);
        ptr->~Decay();
    }
private:
    using CanCopy = typename std::is_copy_constructible<Decay>::type;

    static void copy_selector(Decay const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move only type");
    }

    static void copy_selector(Decay const *src, variant_type_storage &storage, std::true_type)
    {
        new (&storage.buffer) Decay(*src);
    }
};

template<typename T>
struct function_table_selector<T, true, true>
{
    static MetaType_ID type()
    {
        return metaTypeId<Unwrap>();
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
    using Wrapper = decay_t<T>;
    using Unwrap = unwrap_reference_t<Wrapper>;
    using IsArray = is_array_t<Unwrap>;
    using Decay = conditional_t<IsArray::value, remove_cv_t<Unwrap>, full_decay_t<Unwrap>>;

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

    static MetaType_ID type()
    {
        return metaTypeId<remove_cv_t<T>>();
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
        storage.ptr = new Decay(std::move(*ptr));
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
    using CanCopy = typename std::is_copy_constructible<Decay>::type;

    static void copy_selector(Decay const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move-only type");
    }

    static void copy_selector(Decay const *src, variant_type_storage &storage, std::true_type)
    {
        storage.ptr = new Decay(*src);
    }
};

template<typename T, std::size_t N>
struct function_table_selector<T[N], false, false>
{
    using Decay = full_decay_t<T>;
    using Allocator = std::allocator<Decay>;

    static MetaType_ID type()
    {
        return metaTypeId<remove_cv_t<T>[N]>();
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return &value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto from = static_cast<Decay const*>(value);
        copy_selector(from, storage, CanCopy{});
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible<Decay>::value)
    {
        auto alloc = Allocator{};
        auto to = alloc.allocate(N);
        auto from = static_cast<Decay*>(value);
        std::move(from, from + N, to);
        storage.ptr = to;
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto from = static_cast<Decay const*>(src.ptr);
        copy_selector(from, dst, CanCopy{});
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Decay>::value)
    {
        auto alloc = Allocator{};
        auto ptr = static_cast<Decay*>(value.ptr);
        std::_Destroy(ptr, ptr + N);
        alloc.deallocate(ptr, N);
    }

private:
    using CanCopy = typename std::is_copy_constructible<Decay>::type;

    static void copy_selector(Decay const*, variant_type_storage&, std::false_type)
    {
        throw runtime_error("Trying to copy move-only type");
    }

    static void copy_selector(Decay const *src, variant_type_storage &storage, std::true_type)
    {
        auto alloc = Allocator{};
        auto to = alloc.allocate(N);
        std::copy(src, src + N, to);
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
    using Unwrap = unwrap_reference_t<T>;
    using Decay = conditional_t<std::is_array<Unwrap>::value, void, full_decay_t<Unwrap>>;
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
        return ClassInfo{metaTypeId<Decay>(), Selector::access(value)};
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
        [] () noexcept -> MetaType_ID { return MetaType_ID(); },
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
        static_assert(valid, "The contained type must be at least MoveConstructible");
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
    void swap(variant &other) noexcept;

    void clear() noexcept;
    bool empty() const noexcept;
    explicit operator bool() const noexcept
    { return !empty(); }

    MetaType_ID typeId() const
    { return manager->f_type(); }
    ClassInfo classInfo() const
    { return manager->f_info(storage); }

    template<typename T>
    bool is() const
    {
        return metafunc_is<T>::invoke(*this);
    }

    template<typename T>
    T const& value() const &
    {
        using U = remove_reference_t<T>;
        auto const *result = metafunc_cast<U const>::invoke(*this);
        return *result;
    }

    template<typename T>
    T& value() &
    {
        using U = remove_reference_t<T>;
        auto *result = metafunc_cast<U>::invoke(*this);
        return *result;
    }

    template<typename T>
    T const& cvalue() const
    {
        using U = remove_reference_t<T>;
        auto const *result = metafunc_cast<U const>::invoke(*this);
        return *result;
    }

    template<typename T>
    T&& value() &&
    {
        using U = remove_reference_t<T>;
        auto *result = metafunc_cast<U>::invoke(*this);
        return std::move(*result);
    }

    template<typename T>
    T to() const
    {
        static_assert(!std::is_reference<T>::value,
                      "Type cannot be reference");
        static_assert(std::is_move_constructible<T>::value,
                      "Type should be MoveConstructible");

        alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
        metafunc_to<T>::invoke(*this, &buffer);
        return std::move(*reinterpret_cast<T*>(&buffer));
    }

    template<typename T>
    void convert()
    { *this = to<T>(); }

    static variant const empty_variant;
private:
    void const* raw_data_ptr() const
    { return manager->f_access(storage); }
    void * raw_data_ptr()
    { return const_cast<void*>(manager->f_access(storage)); }

    void constructor_selector(void *value, std::true_type)
    { manager->f_move_construct(value, storage); }
    void constructor_selector(void const *value, std::false_type)
    { manager->f_copy_construct(value, storage); }

    template<typename T>
    struct metafunc_is
    {
        static bool invoke(variant const &self)
        {
            if (self.empty())
                return false;

            auto from = MetaType{self.typeId()};
            auto to = MetaType{metaTypeId<T>()};

            if (from.decayId() == to.decayId())
                return MetaType::constCompatible(from, to);

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
            if (from.isClass() && MetaType::constCompatible(from, to))
                return invoke_imp(self);
            return false;
        }
        // class ptr
        static bool cast_selector(variant const &self, MetaType from, MetaType to,
                                  std::false_type, std::true_type)
        {
            if (from.isClassPtr() && MetaType::constCompatible(from, to))
                return  invoke_imp(self);
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

        static T* invoke(variant const &self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};

            Decay const *result = nullptr;
            auto from = MetaType{self.typeId()};
            auto to = MetaType{metaTypeId<add_lvalue_reference_t<T>>()};
            if (from.decayId() == to.decayId())
            {
                if (MetaType::constCompatible(from, to))
                    result = static_cast<Decay const*>(self.raw_data_ptr());
                else
                    throw bad_variant_cast{std::string{"Const incompatible types: "} +
                                           from.typeName() + " -> " + to.typeName()};
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
            if (from.isClass())
            {
                if (!MetaType::constCompatible(from, to))
                    throw bad_variant_cast{std::string{"Const incompatible types: "} +
                                           from.typeName() + " -> " + to.typeName()};

                return invoke_imp(self);
            }
            return nullptr;
        }
        // class ptr
        static void const* cast_selector(variant const &self, MetaType from, MetaType to,
                                         std::false_type, std::true_type)
        {
            if (from.isClassPtr())
            {
                if (!MetaType::constCompatible(from, to))
                    throw bad_variant_cast{std::string{"Const incompatible types: "} +
                                           from.typeName() + " -> " + to.typeName()};

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

        static void invoke(variant const &self, void *buffer)
        {
            assert(buffer);
            if (self.empty())
                throw bad_variant_convert{"Variant is empty"};

            auto from = MetaType{self.typeId()};
            auto to = MetaType{metaTypeId<T>()};
            if (from.decayId() == to.decayId())
            {
                if (MetaType::constCompatible(from, to))
                {
                    Decay const *value = static_cast<Decay const *>(self.raw_data_ptr());
                    new (buffer) Decay(*value);
                    return;
                }
                else
                    throw bad_variant_convert{std::string{"Const incompatible types: "} +
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
            if (from.isClassPtr())
            {
                if (!MetaType::constCompatible(from, to))
                    throw bad_variant_cast{std::string{"Const incompatible types: "} +
                                           from.typeName() + " -> " + to.typeName()};

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

    friend struct std::hash<rtti::variant>;
};

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
