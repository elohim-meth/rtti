#ifndef METHOD_H
#define METHOD_H

#include <rtti/metaclass.h>
#include <rtti/metatype.h>
#include <rtti/metaerror.h>
#include <rtti/finally.h>

#include <cassert>

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// Variant
//--------------------------------------------------------------------------------------------------------------------------------

namespace internal {

constexpr std::size_t STORAGE_ALIGN = sizeof(void*);
constexpr std::size_t STORAGE_SIZE = sizeof(void*) * 2;

union RTTI_API variant_type_storage
{
    void *ptr;
    std::aligned_storage_t<STORAGE_SIZE, STORAGE_ALIGN> buffer;
};

enum class type_attribute {NONE, LREF, RREF, LREF_CONST};

struct RTTI_API variant_function_table
{
    using type_t = MetaType_ID(*)(type_attribute);
    using access_t = void const* (*) (variant_type_storage const&);
    using copy_construct_t = void (*) (void const*, variant_type_storage&);
    using move_construct_t = void (*) (void*, variant_type_storage&);
    using copy_t = void (*) (variant_type_storage const&, variant_type_storage&);
    using move_t = void (*) (variant_type_storage&, variant_type_storage&);
    using destroy_t = void (*) (variant_type_storage&);
    using compare_eq_t = bool (*) (variant_type_storage const&, void const*);
    using info_t = ClassInfo (*) (variant_type_storage const&);

    type_t const f_type = nullptr;
    access_t const f_access = nullptr;
    copy_construct_t const f_copy_construct = nullptr;
    move_construct_t const f_move_construct = nullptr;
    copy_t const f_copy = nullptr;
    move_t const f_move = nullptr;
    destroy_t const f_destroy = nullptr;
    compare_eq_t const f_compare_eq = nullptr;
    info_t const f_info = nullptr;

    variant_function_table
    (
        type_t type,
        access_t access,
        copy_construct_t copy_construct,
        move_construct_t move_construct,
        copy_t copy,
        move_t move,
        destroy_t destroy,
        compare_eq_t compare_eq,
        info_t info
    ) noexcept
    :
        f_type{type},
        f_access{access},
        f_copy_construct{copy_construct},
        f_move_construct{move_construct},
        f_copy{copy},
        f_move{move},
        f_destroy{destroy},
        f_compare_eq{compare_eq},
        f_info(info)
    {}
};

} // namespace internal

class argument;

class RTTI_API variant final
{
public:
    variant() noexcept;
    variant(variant const &other);
    variant& operator=(variant const &other);
    variant(variant &&other) noexcept;
    variant& operator=(variant &&other) noexcept;

    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant(T &&value);

    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant& operator=(T &&value);

    ~variant() noexcept;

    void clear() noexcept;
    bool empty() const noexcept;
    explicit operator bool() const noexcept
    { return !empty(); }

    bool operator==(variant const &value) const;

    template<typename T>
    bool operator==(T const &value) const
    {
        return (*this == variant{std::cref(value)});
    }

    template<typename T>
    bool operator!=(T const &value) const
    {
        return !(*this == value);
    }

    template<typename T>
    bool eq(T const &value) const;

    template<typename T>
    bool neq(T const &value) const
    {
        return !eq(value);
    }

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
    T& ref() &
    {
        using U = std::remove_reference_t<T>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& ref() const &
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF_CONST);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& ref() &&
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::NONE);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return std::move(*result);
    }

    template<typename T>
    T&& rref() &&
    {
        using U = std::remove_reference_t<T>;
        auto fromId = internalTypeId(type_attribute::RREF);
        auto toId = metaTypeId<std::add_rvalue_reference_t<U>>();
        auto *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return std::move(*result);
    }

    template<typename T>
    T const& cref()
    {
        using U = std::add_const_t<std::remove_reference_t<T>>;
        auto fromId = internalTypeId(type_attribute::LREF);
        auto toId = metaTypeId<std::add_lvalue_reference_t<U>>();
        auto const *result = metafunc_cast<U>::invoke(*this, fromId, toId);
        return *result;
    }

    template<typename T>
    T const& cref() const
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

    template<typename ...Args>
    variant invoke(std::string_view name, Args&& ...args);
    template<typename ...Args>
    variant invoke(std::string_view name, Args&& ...args) const;

    using type_attribute = internal::type_attribute;
    static variant const empty_variant;
private:
    void swap(variant &other) noexcept;

    // Returns pointer to decayed type
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
    table_t* manager;

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

//--------------------------------------------------------------------------------------------------------------------------------
// Argument
//--------------------------------------------------------------------------------------------------------------------------------

class RTTI_API argument final
{
public:
    argument() noexcept = default;
    argument(argument const&) = delete;
    argument& operator=(argument const&) = delete;
    argument(argument&&) noexcept = default;
    argument& operator=(argument&&) = delete;
    ~argument() noexcept;

    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, argument>>>
    argument(T &&value) noexcept;

    bool empty() const noexcept;
    MetaType_ID typeId() const;

    template<typename T> T value() const;

private:
    bool isVariant() const;

    // rvalue reference
    template<typename T>
    T value(std::integral_constant<int, 0>) const;

    // lvalue const reference
    template<typename T>
    T value(std::integral_constant<int, 1>) const;

    // lvalue reference
    template<typename T>
    T value(std::integral_constant<int, 2>) const;

    // no reference
    template<typename T>
    T value(std::integral_constant<int, 3>) const;

    void *m_data = nullptr;
    mutable void *m_buffer = nullptr;
    mutable MetaType m_type = {};
};

//--------------------------------------------------------------------------------------------------------------------------------
// MetaMethod
//--------------------------------------------------------------------------------------------------------------------------------

struct RTTI_API IMethodInvoker
{
    enum {
        MaxNumberOfArguments = 10
    };

    virtual bool isStatic() const = 0;
    virtual MetaType_ID returnTypeId() const = 0;
    virtual std::vector<MetaType_ID> parametersTypeId() const = 0;
    virtual std::string signature(std::string_view name) const = 0;
    virtual variant invoke_static(argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual variant invoke_method(const variant &instance,
                                  argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual variant invoke_method(variant &instance,
                                  argument arg0 = argument{}, argument arg1 = argument{},
                                  argument arg2 = argument{}, argument arg3 = argument{},
                                  argument arg4 = argument{}, argument arg5 = argument{},
                                  argument arg6 = argument{}, argument arg7 = argument{},
                                  argument arg8 = argument{}, argument arg9 = argument{}) const = 0;
    virtual ~IMethodInvoker() = default;
};

class MetaMethodPrivate;

class RTTI_API MetaMethod final: public MetaItem
{
    DECLARE_PRIVATE(MetaMethod)
public:
    MetaCategory category() const override;

    template<typename ...Args>
    variant invoke(Args&&... args) const;
protected:
    explicit MetaMethod(std::string_view name, MetaContainer &owner,
                        std::unique_ptr<IMethodInvoker> invoker);
    static MetaMethod* create(std::string_view name, MetaContainer &owner,
                              std::unique_ptr<IMethodInvoker> invoker);
private:
    IMethodInvoker const* invoker() const;

    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    static MetaMethod* create(std::string_view name, MetaContainer &owner,
                              std::unique_ptr<IMethodInvoker> invoker, CreateAccessKey)
    { return create(name, owner, std::move(invoker)); }

};

} // namespace rtti

//--------------------------------------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------------------------------------

#include <rtti/detail/variant_impl.h>
#include <rtti/detail/argument_impl.h>
#include <rtti/detail/metamethod_impl.h>

#endif // METHOD_H

