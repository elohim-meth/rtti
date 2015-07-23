#ifndef VARIANT_H
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
struct class_info_get
{
    static ClassInfo info(const variant_type_storage &value)
    {
        return info_selector(value, is_class_t(), is_class_ptr_t());
    }
private:
    using selector_t = function_table_selector<T>;
    using is_class_t = typename std::is_class<T>::type;
    using is_class_ptr_t = typename internal::is_class_ptr<T>::type;
    using class_t = typename std::remove_pointer<T>::type;

    static ClassInfo info_selector(const variant_type_storage&, std::false_type, std::false_type)
    {
        return ClassInfo();
    }
    static ClassInfo info_selector(const variant_type_storage &value, std::true_type, std::false_type)
    {
        return ClassInfo(selector_t::type(), selector_t::access(value));
    }
    static ClassInfo info_selector(const variant_type_storage &value, std::false_type, std::true_type)
    {
        using registered_t = typename has_method_classInfo<ClassInfo(class_t::*)() const>::type;
        return info_selector_registered(value, registered_t());
    }
    static ClassInfo info_selector_registered(const variant_type_storage&, std::false_type)
    {
        return ClassInfo();
    }
    static ClassInfo info_selector_registered(const variant_type_storage &value, std::true_type)
    {
        auto ptr = reinterpret_cast<class_t**>(selector_t::access(value));
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
        return metafunc_is<internal::full_decay_t<T>>::invoke(*this);
    }

    template<typename T>
    const T& value() const &
    {
        return metafunc_cast<internal::full_decay_t<T>>::invoke(*this);
    }

    template<typename T>
    T& value() &
    {
        return metafunc_cast<internal::full_decay_t<T>>::invoke(*this);
    }

    template<typename T>
    T&& value() &&
    {
        return metafunc_cast<internal::full_decay_t<T>>::invoke(std::move(*this));
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
        : manager{internal::function_table_for<internal::full_decay_t<T>>()}
    {
        new (&storage.buffer) internal::decay_t<T>(std::forward<T>(value));
    }

    template<typename T>
    variant(T &&value, std::false_type)
        : manager{internal::function_table_for<internal::full_decay_t<T>>()}
    {
        storage.ptr = new internal::full_decay_t<T>(std::forward<T>(value));
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
            if (self.type() == metaTypeId<T>())
                return true;
            return invoke_for_class(self, is_class_t(), is_class_ptr_t());
        }

    private:
        using is_class_t = typename std::is_class<T>::type;
        using is_class_ptr_t = typename internal::is_class_ptr<T>::type;
        using class_t = typename std::remove_pointer<T>::type;

        // nope
        static bool invoke_for_class(const variant&, std::false_type, std::false_type)
        { return false; }
        // class
        static bool invoke_for_class(const variant &self, std::true_type, std::false_type)
        {
            auto type = MetaType{self.type()};
            if (type.typeFlags() & MetaType::Class)
                return invoke_imp(self);
            return false;
        }
        // class ptr
        static bool invoke_for_class(const variant &self, std::false_type, std::true_type)
        {
            auto type = MetaType{self.type()};
            if (type.typeFlags() & MetaType::ClassPtr)
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
            auto toClass = MetaClass::findByTypeId(metaTypeId<class_t>());
            if (!fromClass && !toClass)
                return false;
            return fromClass->inheritedFrom(toClass);
        }

    };

    template<typename T>
    struct metafunc_cast
    {
        struct helper
        {
            helper(void *result) noexcept
                : m_result(result), m_resultptr(&m_result)
            {}
            T* result_selector(std::false_type) const noexcept
            {
                return static_cast<T*>(m_result);
            }
            T* result_selector(std::true_type) const noexcept
            {
                return static_cast<T*>(m_resultptr);
            }
        private:
            void* m_result;
            void* m_resultptr;
        };

        static const T& invoke(const variant &self)
        {
            if (self.empty())
                throw bad_variant_cast{"Variant is empty"};
            if (!self.is<T>())
                throw bad_variant_cast{"Incompatible types"};

            T* result = nullptr;
            if (self.type() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               const auto &tmp = invoke_for_class(self, is_class_t(), is_class_ptr_t());
               result = tmp.result_selector(is_class_ptr_t());
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
            if (self.type() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               const auto &tmp = invoke_for_class(self, is_class_t(), is_class_ptr_t());
               result = tmp.result_selector(is_class_ptr_t());
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
            if (self.type() == metaTypeId<T>())
               result = static_cast<T*>(self.raw_data_ptr());
            else
            {
               const auto &tmp = invoke_for_class(self, is_class_t(), is_class_ptr_t());
               result = tmp.result_selector(is_class_ptr_t());
            }

            assert(result);
            return std::move(*result);
        }
    private:
        using is_class_t = typename std::is_class<T>::type;
        using is_class_ptr_t = typename internal::is_class_ptr<T>::type;
        using class_t = typename std::remove_pointer<T>::type;

        // nope
        static helper invoke_for_class(const variant&, std::false_type, std::false_type)
        { return nullptr; }
        // class
        static helper invoke_for_class(const variant &self, std::true_type, std::false_type)
        {
            auto type = MetaType{self.type()};
            if (type.typeFlags() & MetaType::Class)
                return invoke_imp(self);
            return nullptr;
        }
        // class ptr
        static helper invoke_for_class(const variant &self, std::false_type, std::true_type)
        {
            auto type = MetaType{self.type()};
            if (type.typeFlags() & MetaType::ClassPtr)
                return invoke_imp(self);
            return nullptr;
        }
        // implementaion
        static void* invoke_imp(const variant &self)
        {
            const auto &info = self.manager->f_info(self.storage);

            auto fromType = MetaType{info.typeId};
            if (!fromType.valid())
                return false;

            auto fromClass = MetaClass::findByTypeId(info.typeId);
            auto toClass = MetaClass::findByTypeId(metaTypeId<class_t>());
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

        auto type = rtti::MetaType{value.type()};
        return _Hash_impl::hash(value.raw_data_ptr(), type.typeSize());
    }
};

} //namespace std

#endif // VARIANT_H
