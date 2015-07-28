﻿#ifndef VARIANT_H
#define VARIANT_H

#include "metatype.h"
#include "metaerror.h"
#include "metaclass.h"

#include <cassert>

namespace rtti {

// forward
class argument;
class variant;

namespace internal {

constexpr std::size_t STORAGE_SIZE = sizeof(void*) * 2;

template<typename T,
         bool Small = sizeof(decay_t<T>) <= STORAGE_SIZE,
         bool Safe = std::is_move_constructible<T>::value>
using is_inplace = std::integral_constant<bool, Small && Safe>;

template<typename T>
struct is_reference_wrapper: std::false_type
{};

template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>>: std::true_type
{};

template<typename T>
using is_reference_wrapper_t = typename is_reference_wrapper<T>::type;

template<typename T, bool = is_reference_wrapper<T>::value> struct unwrap_reference;

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
        mutable void *temp;
    };
};

struct DLL_PUBLIC variant_function_table
{
    using type_t = MetaType_ID(*)();
    using access_t = void* (*) (const variant_type_storage&);
    using clone_t = void (*) (const variant_type_storage&, variant_type_storage&);
    using move_t = void (*) (variant_type_storage&, variant_type_storage&);
    using destroy_t = void (*) (variant_type_storage&);
    using info_t = ClassInfo(*)(const variant_type_storage&);

    const type_t f_type = nullptr;
    const access_t f_access = nullptr;
    const clone_t f_clone = nullptr;
    const move_t f_move = nullptr;
    const destroy_t f_destroy = nullptr;
    const info_t f_info = nullptr;

    variant_function_table(type_t type, access_t access,
                           clone_t clone, move_t move,
                           destroy_t destroy, info_t info) noexcept
        : f_type{type}, f_access{access},
          f_clone{clone}, f_move{move},
          f_destroy{destroy}, f_info(info)
    {}
};

template<typename T,
         bool = is_inplace<T>::value,
         bool = is_reference_wrapper<T>::value>
struct function_table_selector;

template<typename T>
struct function_table_selector<T, true, false>
{
    using Decay = full_decay_t<T>;

    static MetaType_ID type() noexcept
    {
        return metaTypeId<T>();
    }

    static void* access(const variant_type_storage &value) noexcept
    {
        auto ptr = reinterpret_cast<const Decay*>(&value.buffer);
        return const_cast<Decay*>(ptr);
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

    static void destroy(variant_type_storage &value) noexcept
    {
        auto ptr = reinterpret_cast<const Decay*>(&value.buffer);
        ptr->~Decay();
    }
};

template<typename T>
struct function_table_selector<T, true, true>
{
    static MetaType_ID type() noexcept
    {
        return metaTypeId<Unwrap>();
    }

    static void* access(const variant_type_storage &value) noexcept
    {
        auto ptr = reinterpret_cast<const T*>(&value.buffer);
        return const_cast<Decay*>(&ptr->get());
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
private:
    using Unwrap = unwrap_reference_t<T>;
    using Decay = full_decay_t<Unwrap>;
};


template<typename T>
struct function_table_selector<T, false, false>
{
    using Decay = full_decay_t<T>;

    static MetaType_ID type() noexcept
    {
        return metaTypeId<T>();
    }

    static void* access(const variant_type_storage &value) noexcept
    {
        auto ptr = static_cast<const Decay*>(value.ptr);
        return const_cast<Decay*>(ptr);
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

    static void destroy(variant_type_storage &value) noexcept
    {
        auto ptr = static_cast<const Decay*>(value.ptr);
        delete ptr;
    }
};

template<typename T>
struct class_info_get
{
    static ClassInfo info(const variant_type_storage &value)
    {
        return info_selector(value, IsClass{}, IsClassPtr{});
    }
private:
    using Unwrap = full_decay_t<unwrap_reference_t<T>>;
    using Selector = function_table_selector<T>;
    using IsClass = is_class_t<Unwrap>;
    using IsClassPtr = is_class_ptr_t<Unwrap>;
    using C = remove_pointer_t<Unwrap>;

    static ClassInfo info_selector(const variant_type_storage&, std::false_type, std::false_type)
    {
        return ClassInfo();
    }
    static ClassInfo info_selector(const variant_type_storage &value, std::true_type, std::false_type)
    {
        return ClassInfo(Selector::type(), Selector::access(value));
    }
    static ClassInfo info_selector(const variant_type_storage &value, std::false_type, std::true_type)
    {
        using registered_t = typename has_method_classInfo<ClassInfo(C::*)() const>::type;
        return info_selector_registered(value, registered_t());
    }
    static ClassInfo info_selector_registered(const variant_type_storage&, std::false_type)
    {
        return ClassInfo();
    }
    static ClassInfo info_selector_registered(const variant_type_storage &value, std::true_type)
    {
        auto ptr = reinterpret_cast<C**>(Selector::access(value));
        return (*ptr)->classInfo();
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
        &function_table_selector<T>::destroy,
        &class_info_get<T>::info
    };
    return &result;
}

template<>
inline const variant_function_table* function_table_for<void>()
{
    static const auto result = variant_function_table{
        [] () noexcept -> MetaType_ID { return MetaType_ID(); },
        [] (const variant_type_storage&) noexcept -> void* { return nullptr; },
        [] (const variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {},
        [] (const variant_type_storage&) { return ClassInfo(); }
    };
    return &result;
}

} // namespace internal

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
                 !std::is_same<variant, decay_t<T>>::value>
             ::type>
    variant(T &&value)
        : variant{std::forward<T>(value), internal::is_inplace<T>{}}
    {
        static constexpr bool valid = std::is_copy_constructible<T>::value;
        static_assert(valid, "The contained object must be CopyConstructible");
    }

    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<variant, decay_t<T>>::value>
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

    MetaType_ID typeId() const noexcept
    {
        return manager->f_type();
    }

    ClassInfo classInfo() const noexcept
    {
        return manager->f_info(storage);
    }

    template<typename T>
    bool is() const
    {
        return metafunc_is<full_decay_t<T>>::invoke(*this);
    }

    template<typename T>
    const T& value() const &
    {
        auto &result = metafunc_cast<full_decay_t<T>>::invoke(*this);
        return const_cast<const T&>(result);
    }

    template<typename T>
    T& value() &
    {
        auto &result = metafunc_cast<full_decay_t<T>>::invoke(*this);
        return const_cast<T&>(result);
    }

    template<typename T>
    T&& value() &&
    {
        auto &&result = metafunc_cast<full_decay_t<T>>::invoke(std::move(*this));
        return const_cast<T&&>(result);
    }

    template<typename T>
    T to() const
    {
        static_assert(!std::is_reference<T>::value,
                      "Type cannot be reference");
        static_assert(std::is_move_constructible<T>::value,
                      "Type should be MoveConstructible");

        if (is<T>())
            return value<T>();

        auto fromId = typeId();
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
    bool convert()
    {
        if (is<T>())
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
    template<typename T>
    variant(T &&value, std::true_type)
        : manager{internal::function_table_for<remove_reference_t<T>>()}
    {
        new (&storage.buffer) decay_t<T>(std::forward<T>(value));
    }

    template<typename T>
    variant(T &&value, std::false_type)
        : manager{internal::function_table_for<remove_reference_t<T>>()}
    {
        storage.ptr = new decay_t<T>(std::forward<T>(value));
        storage.temp = nullptr;
    }

    void* raw_data_ptr() const noexcept
    {
        return manager->f_access(storage);
    }

    template<typename T>
    struct metafunc_is
    {
        static_assert(!std::is_reference<T>::value,
                      "Type cannot be reference");

        static bool invoke(const variant &self)
        {
            if (self.empty())
                return false;
            auto type = MetaType{self.typeId()};
            if (type.decayId() == metaTypeId<T>())
                return true;
            return invoke_for_class(self, IsClass{}, IsClassPtr{});
        }

    private:
        using IsClass = is_class_t<T>;
        using IsClassPtr = is_class_ptr_t<T>;
        using C = remove_pointer_t<T>;

        // nope
        static bool invoke_for_class(const variant&, std::false_type, std::false_type)
        { return false; }
        // class
        static bool invoke_for_class(const variant &self, std::true_type, std::false_type)
        {
            auto type = MetaType{self.typeId()};
            if (type.isClass())
                return invoke_imp(self);
            return false;
        }
        // class ptr
        static bool invoke_for_class(const variant &self, std::false_type, std::true_type)
        {
            auto type = MetaType{self.typeId()};
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
        static const T& invoke(const variant &self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};
            if (!self.is<T>())
                throw bad_variant_cast{"Incompatible types"};

            T* result = nullptr;
            auto type = MetaType{self.typeId()};
            if (type.decayId() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               auto ptr = invoke_for_class(self, IsClass{}, IsClassPtr{});
               result = static_cast<T*>(ptr);
            }

            assert(result);
            return *result;
        }

        static T& invoke(variant &self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};
            if (!self.is<T>())
                throw bad_variant_cast{"Incompatible types"};

            T* result = nullptr;
            auto type = MetaType{self.typeId()};
            if (type.decayId() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               auto ptr = invoke_for_class(self, IsClass{}, IsClassPtr{});
               result = static_cast<T*>(ptr);
            }

            assert(result);
            return *result;
        }

        static T&& invoke(variant &&self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};
            if (!self.is<T>())
                throw bad_variant_cast{"Incompatible types"};

            T* result = nullptr;
            auto type = MetaType{self.typeId()};
            if (type.decayId() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               auto ptr = invoke_for_class(self, IsClass{}, IsClassPtr{});
               result = static_cast<T*>(ptr);
            }

            assert(result);
            return std::move(*result);
        }
    private:
        using IsClass = is_class_t<T>;
        using IsClassPtr = is_class_ptr_t<T>;
        using C = remove_pointer_t<T>;

        // nope
        static void* invoke_for_class(const variant&, std::false_type, std::false_type)
        { return nullptr; }
        // class
        static void* invoke_for_class(const variant &self, std::true_type, std::false_type)
        {
            auto type = MetaType{self.typeId()};
            if (type.isClass())
                return invoke_imp(self);
            return nullptr;
        }
        // class ptr
        static void* invoke_for_class(const variant &self, std::false_type, std::true_type)
        {
            auto type = MetaType{self.typeId()};
            if (type.isClassPtr())
            {
                auto ptr = invoke_imp(self);
                self.storage.temp = ptr;
                return &self.storage.temp;
            }
            return nullptr;
        }
        // implementaion
        static void* invoke_imp(const variant &self)
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

    table_t* manager = internal::function_table_for<void>();
    storage_t storage = {.buffer = {0}};

    friend class std::hash<rtti::variant>;
    friend class rtti::argument;
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
        return _Hash_impl::hash(value.raw_data_ptr(), type.typeSize());
    }
};

} //namespace std

#endif // VARIANT_H
