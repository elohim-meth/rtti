#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"
#include "metaclass.h"

#include <cassert>

namespace rtti {

namespace internal {

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
    alignas(void*) std::uint8_t buffer[STORAGE_SIZE];
    struct {
        void *ptr;
        mutable const void *temp;
    };
    variant_type_storage(): buffer{0} {}
};

struct DLL_PUBLIC variant_function_table
{
    using type_t = MetaType_ID(*)();
    using access_t = const void* (*) (const variant_type_storage&);
    using construct_t = void (*) (variant_type_storage&, const void*, bool);
    using clone_t = void (*) (const variant_type_storage&, variant_type_storage&);
    using move_t = void (*) (variant_type_storage&, variant_type_storage&);
    using destroy_t = void (*) (variant_type_storage&);
    using info_t = ClassInfo(*)(const variant_type_storage&);

    const type_t f_type = nullptr;
    const access_t f_access = nullptr;
    const construct_t f_construct = nullptr;
    const clone_t f_clone = nullptr;
    const move_t f_move = nullptr;
    const destroy_t f_destroy = nullptr;
    const info_t f_info = nullptr;

    variant_function_table(type_t type, access_t access,
                           construct_t construct, clone_t clone,
                           move_t move, destroy_t destroy,
                           info_t info) noexcept
        : f_type{type}, f_access{access},
          f_construct{construct}, f_clone{clone},
          f_move{move}, f_destroy{destroy},
          f_info(info)
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

    static void construct(variant_type_storage &storage, const void *value, bool move)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        if (move)
        {
            auto ptr = static_cast<Decay*>(const_cast<void*>(value));
            new (&storage.buffer) Decay(std::move(*ptr));
        }
        else
        {
            auto ptr = static_cast<const Decay*>(value);
            new (&storage.buffer) Decay(*ptr);
        }
    }

    static const void* access(const variant_type_storage &value) noexcept
    {
        return &value.buffer;
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = reinterpret_cast<const Decay*>(&src.buffer);
        new (&dst.buffer) Decay(*ptr);
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
        auto ptr = reinterpret_cast<const Decay*>(&value.buffer);
        ptr->~Decay();
    }
};

template<typename T>
struct function_table_selector<T, true, true>
{
    static MetaType_ID type()
    {
        return metaTypeId<Unwrap>();
    }

    static const void* access(const variant_type_storage &value) noexcept
    {
        return access_selector(value, IsArray{});
    }

    static void construct(variant_type_storage &storage, const void *value, bool) noexcept
    {
        auto ptr = static_cast<const Wrapper*>(value);
        storage.ptr = const_cast<Decay*>(&ptr->get());
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        dst.ptr = src.ptr;
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
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

    static const void* access_selector(const variant_type_storage &value, std::false_type) noexcept
    {
        return value.ptr;
    }

    static const void* access_selector(const variant_type_storage &value, std::true_type) noexcept
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

    static const void* access(const variant_type_storage &value) noexcept
    {
        return value.ptr;
    }

    static void construct(variant_type_storage &storage, const void *value, bool move)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        if (move)
        {
            auto ptr = static_cast<Decay*>(const_cast<void*>(value));
            storage.ptr = new Decay(std::move(*ptr));
        }
        else
        {
            auto ptr = static_cast<const Decay*>(value);
            storage.ptr = new Decay(*ptr);
        }
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto ptr = static_cast<const Decay*>(src.ptr);
        dst.ptr = new Decay(*ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value)
        noexcept(std::is_nothrow_destructible<Decay>::value)
    {
        auto ptr = static_cast<const Decay*>(value.ptr);
        delete ptr;
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

    static const void* access(const variant_type_storage &value) noexcept
    {
        return &value.ptr;
    }

    static void construct(variant_type_storage &storage, const void *value, bool move)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto alloc = Allocator{};
        auto to = alloc.allocate(N);
        if (move)
        {
            auto from = static_cast<Decay*>(const_cast<void*>(value));
            std::move(from, from + N, to);
        }
        else
        {
            auto from = static_cast<const Decay*>(value);
            std::copy(from, from + N, to);
        }
        storage.ptr = to;
    }

    static void clone(const variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible<Decay>::value)
    {
        auto from = static_cast<const Decay*>(src.ptr);
        auto to = static_cast<Decay*>(::operator new(N * sizeof(T)));
        std::copy(from, from + N, to);
        dst.ptr = to;
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
    static ClassInfo info(const variant_type_storage &value)
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

    static ClassInfo info_selector(const variant_type_storage&, std::false_type, std::false_type)
    {
        return ClassInfo{};
    }

    static ClassInfo info_selector(const variant_type_storage &value, std::true_type, std::false_type)
    {
        using IsRegistered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
        return info_selector_registered(value, IsRegistered{});
    }
    static ClassInfo info_selector_registered(const variant_type_storage &value, std::false_type)
    {
        return ClassInfo{metaTypeId<Decay>(), Selector::access(value)};
    }
    static ClassInfo info_selector_registered(const variant_type_storage &value, std::true_type)
    {
        auto ptr = static_cast<const C*>(Selector::access(value));
        return ptr->classInfo();
    }

    static ClassInfo info_selector(const variant_type_storage &value, std::false_type, std::true_type)
    {
        using IsRegistered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
        return info_selector_registered_ptr(value, IsRegistered{});
    }
    static ClassInfo info_selector_registered_ptr(const variant_type_storage &value, std::false_type)
    {
        auto ptr = reinterpret_cast<C const * const *>(Selector::access(value));
        return ClassInfo{metaTypeId<C>(), *ptr};
    }
    static ClassInfo info_selector_registered_ptr(const variant_type_storage &value, std::true_type)
    {
        auto ptr = reinterpret_cast<C const * const *>(Selector::access(value));
        return (*ptr)->classInfo();
    }
};

template<typename T>
inline const variant_function_table* function_table_for() noexcept
{
    static const auto result = variant_function_table{
        &function_table_selector<T>::type,
        &function_table_selector<T>::access,
        &function_table_selector<T>::construct,
        &function_table_selector<T>::clone,
        &function_table_selector<T>::move,
        &function_table_selector<T>::destroy,
        &class_info_get<T>::info
    };
    return &result;
}

template<>
inline const variant_function_table* function_table_for<void>() noexcept
{
    static const auto result = variant_function_table{
        [] () noexcept -> MetaType_ID { return MetaType_ID(); },
        [] (const variant_type_storage&) noexcept -> const void* { return nullptr; },
        [] (variant_type_storage&, const void*, bool) noexcept {},
        [] (const variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {},
        [] (const variant_type_storage&) noexcept { return ClassInfo(); }
    };
    return &result;
}

} // namespace internal

class DLL_PUBLIC variant final
{
public:
    variant() = default;

    variant(const variant &other);
    variant& operator=(const variant &other);
    variant(variant &&other) noexcept;
    variant& operator=(variant &&other) noexcept;

    template<typename T,
             typename = enable_if_t<!std::is_same<decay_t<T>, variant>::value>>
    variant(T &&value)
        : manager{internal::function_table_for<remove_reference_t<T>>()}
    {
        using NoRef = remove_reference_t<T>;
        using Type = conditional_t<std::is_array<NoRef>::value, remove_all_extents_t<NoRef>, NoRef>;
        static constexpr bool valid = std::is_copy_constructible<Type>::value;
        static_assert(valid, "The contained type must be CopyConstructible");
        manager->f_construct(storage, std::addressof(value),
                             !std::is_reference<T>::value && !std::is_const<T>::value);
    }

    template<typename T,
             typename = enable_if_t<!std::is_same<decay_t<T>, variant>::value>>
    variant& operator=(T &&value)
    {
        variant{std::forward<T>(value)}.swap(*this);
        return *this;
    }

    ~variant();
    void swap(variant &other) noexcept;

    void clear()
    { variant{}.swap(*this); }
    bool empty() const;
    explicit operator bool() const
    { return !empty(); }

    MetaType_ID typeId() const
    { return manager->f_type(); }
    ClassInfo classInfo() const
    { return manager->f_info(storage); }

    template<typename T>
    bool is() const
    {
        return metafunc_is<T>::invoke(*this, false);
    }

    template<typename T>
    const T& value() const &
    {
        using U = remove_reference_t<T>;
        const auto *result = metafunc_cast<const U>::invoke(*this);
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
    const T& cvalue() const
    {
        using U = remove_reference_t<T>;
        const auto *result = metafunc_cast<const U>::invoke(*this);
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

        if (metafunc_is<T>::invoke(*this, true))
            return cvalue<T>();

        auto from = MetaType{typeId()};
        auto to = MetaType{metaTypeId<T>()};
        if (MetaType::hasConverter(from, to))
        {
            alignas(T) std::uint8_t buffer[sizeof(T)] = {0};
            if (MetaType::convert(raw_data_ptr(), from, &buffer, to))
                return std::move(*reinterpret_cast<T*>(&buffer));
            throw bad_variant_convert{std::string{"Conversion failed: "} +
                                      from.typeName() + " -> " + to.typeName()};

        }
        throw bad_variant_convert{std::string{"Converter not found: "} +
                                  from.typeName() + " -> " + to.typeName()};
    }

    template<typename T>
    bool convert()
    {
        if (metafunc_is<T>::invoke(*this, false))
            return true;

        auto fromId = typeId();
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
    const void* raw_data_ptr() const
    { return manager->f_access(storage); }
    void* raw_data_ptr()
    { return const_cast<void*>(manager->f_access(storage)); }

    static bool isConstCompatible(MetaType from, MetaType to, bool raise)
    {
        if (!MetaType::constCompatible(from, to))
        {
            if (raise)
                throw bad_variant_cast{std::string{"Const incompatible types: "} +
                                       from.typeName() + " -> " + to.typeName()};
            return false;
        }
        return true;
    }

    template<typename T>
    struct metafunc_is
    {
        static bool invoke(const variant &self, bool raise)
        {
            if (self.empty())
            {
                if (raise)
                    throw bad_variant_cast{"Variant is empty"};
                return false;
            }

            auto from = MetaType{self.typeId()};
            auto to = MetaType{metaTypeId<T>()};

            if (from.decayId() == to.decayId())
                return isConstCompatible(from, to, raise);

            if (invoke_selector(self, from, IsClass{}, IsClassPtr{}))
                return isConstCompatible(from, to, raise);

            return false;
        }

    private:
        using Decay = full_decay_t<T>;
        using IsClass = is_class_t<Decay>;
        using IsClassPtr = is_class_ptr_t<Decay>;
        using C = remove_pointer_t<Decay>;

        // nope
        static bool invoke_selector(const variant&, MetaType,
                                    std::false_type, std::false_type)
        { return false; }
        // class
        static bool invoke_selector(const variant &self, MetaType type,
                                    std::true_type, std::false_type)
        {
            if (type.isClass())
                return invoke_imp(self);
            return false;
        }
        // class ptr
        static bool invoke_selector(const variant &self, MetaType type,
                                    std::false_type, std::true_type)
        {
            if (type.isClassPtr())
                return invoke_imp(self);
            return false;
        }
        // implementaion
        static bool invoke_imp(const variant &self)
        {
            const auto &info = self.manager->f_info(self.storage);

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

        static T* invoke(const variant &self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};

            const Decay* result = nullptr;
            auto from = MetaType{self.typeId()};
            auto to = MetaType{metaTypeId<add_lvalue_reference_t<T>>()};
            if (from.decayId() == to.decayId())
            {
                if (isConstCompatible(from, to, true))
                    result = static_cast<const Decay*>(self.raw_data_ptr());
            }
            else
            {
               auto ptr = invoke_selector(self, from, IsClass{}, IsClassPtr{});
               if (ptr && isConstCompatible(from, to, true))
                   result = static_cast<const Decay*>(ptr);
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

        static T* result_selector(const Decay *value, std::false_type)
        { return const_cast<T*>(value); }
        static T* result_selector(const Decay *value, std::true_type)
        { return reinterpret_cast<T*>(*value); }

        // nope
        static const void* invoke_selector(const variant&, MetaType,
                                     std::false_type, std::false_type)
        { return nullptr; }
        // class
        static const void* invoke_selector(const variant &self, MetaType type,
                                     std::true_type, std::false_type)
        {
            if (type.isClass())
                return invoke_imp(self);
            return nullptr;
        }
        // class ptr
        static const void* invoke_selector(const variant &self, MetaType type,
                                     std::false_type, std::true_type)
        {
            if (type.isClassPtr())
            {
                auto ptr = invoke_imp(self);
                if (ptr == self.storage.ptr)
                    return &self.storage.ptr;
                else
                {
                    self.storage.temp = ptr;
                    return &self.storage.temp;
                }
            }
            return nullptr;
        }
        // implementaion
        static const void* invoke_imp(const variant &self)
        {
            const auto &info = self.manager->f_info(self.storage);

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

    using table_t = const internal::variant_function_table;
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
    typename this_t::result_type operator()(const typename this_t::argument_type &value) const
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
