#ifndef METATYPE_H
#define METATYPE_H

#include "misc_traits.h"
#include "metaerror.h"

#include <typename.h>
#include <tagged_id.h>

#include <limits>

#include "global.h"

namespace rtti {

struct meta_type_tag {};
using MetaType_ID = mpl::ID<meta_type_tag, std::uint32_t,
                       std::numeric_limits<std::uint32_t>::max()>;

/* begin forward */
namespace internal {
struct type_function_table;
template<typename T> struct type_function_table_impl;
template<typename T> class meta_type;
struct ConvertFunctionBase;
} // namespace internal

using metatype_manager_t = internal::type_function_table;
template<typename T>
using type_manager_t = internal::type_function_table_impl<remove_all_cv_t<remove_reference_t<T>>>;

struct TypeInfo;
class MetaClass;
class variant;
class argument;
/* end forward */

class DLL_PUBLIC MetaType final {
public:
    enum : MetaType_ID::type {
        InvalidTypeId = MetaType_ID::Default
    };

    enum TypeFlags: std::uint32_t {
        None                 = 0,

        Const                = 1 << 0,
        Pointer              = 1 << 1,
        MemberPointer        = 1 << 2,
        LvalueReference      = 1 << 3,
        RvalueReference      = 1 << 4,
        Array                = 1 << 5,

        Void                 = 1 << 6,
        Integral             = 1 << 7,
        FloatPoint           = 1 << 8,
        Enum                 = 1 << 9,
        Function             = 1 << 10,
        Union                = 1 << 11,
        Class                = 1 << 12,

        Pod                  = 1 << 13,
        Abstract             = 1 << 14,
        Polymorphic          = 1 << 15,
        DefaultConstructible = 1 << 16,
        CopyConstructible    = 1 << 17,
        CopyAssignable       = 1 << 18,
        MoveConstructible    = 1 << 19,
        MoveAssignable       = 1 << 20,
        Destructible         = 1 << 21,
    };

    MetaType() noexcept = default;
    explicit MetaType(MetaType_ID typeId) noexcept;
    explicit MetaType(char const *name) noexcept;

    bool valid() const noexcept
    {
        return m_typeInfo != nullptr;
    }
    MetaType_ID typeId() const noexcept;
    MetaType_ID decayId() const noexcept;
    bool decayed() const noexcept
    { return valid() && (typeId() == decayId()); }
    void setTypeId(MetaType_ID typeId);
    char const* typeName() const noexcept;
    std::size_t typeSize() const noexcept;
    MetaType::TypeFlags typeFlags() const noexcept;

    bool isConst() const noexcept;
    bool isLvalueReference() const noexcept;
    bool isRvalueReference() const noexcept;
    bool isReference() const noexcept;
    bool isClass() const noexcept;
    bool isPointer() const noexcept;
    bool isClassPtr() const noexcept;
    bool isArray() const noexcept;
    uint16_t pointerArity() const noexcept;
    static bool compatible(MetaType fromType, MetaType toType) noexcept;

    static bool hasConverter(MetaType fromType, MetaType toType) noexcept;
    static bool hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId) noexcept;
    template<typename From, typename To>
    static bool hasConverter() noexcept;

    template<typename From, typename To, typename Func>
    static bool registerConverter(Func &&func);
    template<typename From, typename To>
    static bool registerConverter(To(*func)(From));
    template<typename From, typename To>
    static bool registerConverter();
    template<typename From, typename To>
    static bool registerConverter(To(From::*func)() const);
    template<typename From, typename To>
    static bool registerConverter(To(From::*func)(bool*) const);

    static void unregisterConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId);
    template<typename From, typename To>
    static void unregisterConverter();
private:
    static MetaType_ID registerMetaType(char const *name, std::size_t size,
                                        MetaType_ID decay, uint16_t arity, uint16_t const_mask,
                                        MetaType::TypeFlags flags, metatype_manager_t const *manager);

    void* allocate() const;
    void deallocate(void *ptr) const;
    void default_construct(void *where) const;
    void copy_construct(void const *source, void *where) const;
    void move_construct(void *source, void *where) const;
    void move_or_copy(void *source, bool movable, void *where) const;
    void destroy(void *ptr) const noexcept;

    template<typename From, typename To, typename Func>
    static bool registerConverter_imp(Func &&func);
    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)() const);
    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)(bool*) const);
    static bool registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                  internal::ConvertFunctionBase const &converter);
    static bool convert(void const *from, MetaType fromType, void *to, MetaType toType);
    static bool convert(void const *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId);

    TypeInfo const *m_typeInfo = nullptr;

    template<typename> friend class internal::meta_type;
    friend class rtti::MetaClass;
    friend class rtti::variant;
    friend class rtti::argument;
};

//forward
template <typename T> MetaType_ID metaTypeId();

namespace internal {

template <typename T>
inline T move_or_copy(void *source, bool movable, std::true_type, std::false_type)
{
    if (movable)
        return std::move(*static_cast<T*>(source));

    throw runtime_error("Type T = " + mpl::type_name<T>() + "isn't CopyConstructible");
}

template <typename T>
inline T move_or_copy(void *source, bool, std::false_type, std::true_type)
{
    return *static_cast<T*>(source);
}

template <typename T>
inline T move_or_copy(void *source, bool movable, std::true_type, std::true_type)
{
    if (movable)
        return std::move(*static_cast<T*>(source));
    else
        return *static_cast<T*>(source);
}

template <typename T>
inline T move_or_copy(void *source, bool movable)
{
    static_assert(std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value,
                  "Type should be CopyConstructible or MoveConstructible");

    return move_or_copy<T>(source, movable, is_move_constructible_t<T>{}, is_copy_constructible_t<T>{});
}

struct DLL_LOCAL type_function_table
{
    using allocate_t = void* (*) ();
    using deallocate_t = void (*) (void*);
    using default_construct_t = void (*) (void*);
    using copy_construct_t = void (*) (void const*, void*);
    using move_construct_t = void (*) (void*, void*);
    using move_or_copy_t = void (*) (void*, bool, void*);
    using destroy_t = void (*) (void*);

    allocate_t const f_allocate = nullptr;
    deallocate_t const f_deallocate = nullptr;
    default_construct_t const f_default_construct = nullptr;
    copy_construct_t const f_copy_construct = nullptr;
    move_construct_t const f_move_construct = nullptr;
    move_or_copy_t const f_move_or_copy = nullptr;
    destroy_t const f_destroy = nullptr;

    constexpr type_function_table(
                            allocate_t allocate, deallocate_t deallocate,
                            default_construct_t default_construct,
                            copy_construct_t copy_construct,
                            move_construct_t move_construct,
                            move_or_copy_t move_or_copy,
                            destroy_t destroy) noexcept
        : f_allocate{allocate}, f_deallocate{deallocate},
          f_default_construct{default_construct},
          f_copy_construct{copy_construct},
          f_move_construct{move_construct},
          f_move_or_copy{move_or_copy},
          f_destroy{destroy}
    {}
};

template<typename T>
struct type_function_table_impl
{
    static void* allocate()
    {
        return ::operator new(sizeof(T));
    }

    static void deallocate(void *ptr)
    {
        if (ptr)
            ::operator delete(ptr);
    }

    static void default_construct(void *where)
        noexcept(std::is_nothrow_default_constructible<T>::value)
    {
        default_construct(where, is_default_constructible_t<T>{});
    }

    static void copy_construct(void const *source, void *where)
        noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        copy_construct(source, where, is_copy_constructible_t<T>{});
    }

    static void move_construct(void *source, void *where)
        noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        move_construct(source, where, is_move_constructible_t<T>{});
    }

    static void move_or_copy (void *source, bool movable, void *where)
    {
        move_or_copy(source, movable, where,
                     is_move_constructible_t<T>{},
                     is_copy_constructible_t<T>{});
    }

    static void destroy(void *ptr) noexcept
    {
        destroy(ptr, is_trivially_destructible_t<T>{});
    }
private:
    static void default_construct(void *where, std::true_type)
    {
        default_construct_imp(where, is_trivially_default_constructible_t<T>{});
    }
    static void default_construct(void *, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<T>() + "isn't DefaultConstructible");
    }
    static void default_construct_imp(void *where, std::false_type)
    {
        if (where)
            new (where) T();
    }
    static void default_construct_imp(void*, std::true_type)
    { /* do nothing */ }

    static void move_construct(void *source, void *where, std::true_type)
    {
        if (source && where)
            new (where) T(std::move(*static_cast<T*>(source)));
    }
    static void move_construct(void*, void*, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<T>() + "isn't MoveConstructible");
    }

    static void copy_construct(void const *source, void *where, std::true_type)
    {
        if (source && where)
            new (where) T(*static_cast<T const*>(source));
    }
    static void copy_construct(void const*, void*, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<T>() + "isn't CopyConstructible");
    }

    static void move_or_copy (void*, bool, void*, std::false_type, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<T>() + "isn't Copy or MoveConstructible");
    }
    static void move_or_copy (void *source, bool movable, void *where, std::true_type, std::false_type)
    {
        movable ? move_construct(source, where, std::true_type{})
                : copy_construct(source, where, std::false_type{});
    }
    static void move_or_copy (void *source, bool, void *where, std::false_type, std::true_type)
    {
        copy_construct(source, where, std::true_type{});
    }
    static void move_or_copy (void *source, bool movable, void *where, std::true_type, std::true_type)
    {
        movable ? move_construct(source, where, std::true_type{})
                : copy_construct(source, where, std::true_type{});
    }

    static void destroy(void *ptr, std::false_type) noexcept
    {
        if (ptr)
            static_cast<T*>(ptr)->~T();
    }
    static void destroy(void*, std::true_type) noexcept
    { /* do nothing */ }
};

template<typename T, std::size_t N>
struct type_function_table_impl<T[N]>
{
    using Base = remove_all_extents_t<T[N]>;
    static constexpr auto Length = array_length<T[N]>::value;

    static void* allocate()
    {
        return ::operator new(Length*sizeof(Base));
    }

    static void deallocate(void *ptr)
    {
        if (ptr)
            ::operator delete(ptr);
    }

    static void default_construct(void *where)
        noexcept(std::is_nothrow_default_constructible<Base>::value)
    {
        default_construct(where, is_default_constructible_t<Base>{});
    }

    static void copy_construct(void const *source, void *where)
        noexcept(std::is_nothrow_copy_constructible<Base>::value)
    {
        copy_construct(source, where, is_copy_constructible_t<Base>{});
    }

    static void move_construct(void *source, void *where)
        noexcept(std::is_nothrow_move_constructible<Base>::value)
    {
        move_construct(source, where, is_move_constructible_t<Base>{});
    }

    static void move_or_copy (void *source, bool movable, void *where)
    {
        move_or_copy(source, movable, where,
                     is_move_constructible_t<Base>{},
                     is_copy_constructible_t<Base>{});
    }

    static void destroy(void *ptr) noexcept
    {
        destroy(ptr, is_trivially_destructible_t<Base>{});
    }
private:
    static void default_construct(void *where, std::true_type)
    {
        default_construct_imp(where, is_trivially_default_constructible_t<Base>{});
    }
    static void default_construct(void *, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<Base>() + "isn't DefaultConstructible");
    }
    static void default_construct_imp(void *where, std::false_type)
    {
        if (where)
        {
            auto *begin = static_cast<Base*>(where);
            auto *end = static_cast<Base*>(where) + Length;

            for (; begin != end; ++begin)
                new (begin) Base();
        }
    }
    static void default_construct_imp(void*, std::true_type)
    { /* do nothing */ }

    static void move_construct(void *source, void *where, std::true_type)
    {
        if (source && where)
        {
            auto *it = static_cast<Base*>(source);
            std::move(it, it + Length, static_cast<Base*>(where));
        }
    }
    static void move_construct(void*, void*, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<Base>() + "isn't MoveConstructible");
    }

    static void copy_construct(void const *source, void *where, std::true_type)
    {
        if (source && where)
        {
            auto *it = static_cast<Base const*>(source);
            std::copy(it, it + Length, static_cast<Base*>(where));
        }
    }
    static void copy_construct(void const*, void*, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<Base>() + "isn't CopyConstructible");
    }

    static void move_or_copy (void*, bool, void*, std::false_type, std::false_type)
    {
        throw runtime_error("Type T = " + mpl::type_name<Base>() + "isn't Copy or MoveConstructible");
    }
    static void move_or_copy (void *source, bool movable, void *where, std::true_type, std::false_type)
    {
        movable ? move_construct(source, where, std::true_type{})
                : copy_construct(source, where, std::false_type{});
    }
    static void move_or_copy (void *source, bool, void *where, std::false_type, std::true_type)
    {
        copy_construct(source, where, std::true_type{});
    }
    static void move_or_copy (void *source, bool movable, void *where, std::true_type, std::true_type)
    {
        movable ? move_construct(source, where, std::true_type{})
                : copy_construct(source, where, std::true_type{});
    }

    static void destroy(void *ptr, std::false_type) noexcept
    {
        if (ptr)
        {
            auto *begin = static_cast<Base*>(ptr);
            auto *end = static_cast<Base*>(ptr) + Length;

            for (; begin != end; ++begin)
                begin->~Base();
        }
    }
    static void destroy(void*, std::true_type) noexcept
    { /* do nothing */ }
};

template<typename T>
inline type_function_table const* type_function_table_for() noexcept
{
    static auto const result = type_function_table{
        &type_function_table_impl<T>::allocate,
        &type_function_table_impl<T>::deallocate,
        &type_function_table_impl<T>::default_construct,
        &type_function_table_impl<T>::copy_construct,
        &type_function_table_impl<T>::move_construct,
        &type_function_table_impl<T>::move_or_copy,
        &type_function_table_impl<T>::destroy
    };
    return &result;
}

template <typename T>
struct type_flags {
    using Flags = MetaType::TypeFlags;
    using no_ref = remove_reference_t<T>;
    using no_ptr = remove_pointer_t<no_ref>;
    using base = base_type_t<T>;
    static Flags const value = static_cast<Flags>(
        (std::is_const<no_ref>::value ? Flags::Const : Flags::None) |
        (std::is_pointer<no_ref>::value ? Flags::Pointer : Flags::None) |
        (std::is_member_pointer<no_ref>::value ? Flags::MemberPointer : Flags::None) |
        (std::is_lvalue_reference<T>::value ? Flags::LvalueReference : Flags::None) |
        (std::is_rvalue_reference<T>::value ? Flags::RvalueReference : Flags::None) |
        (std::is_array<no_ptr>::value ? Flags::Array : Flags::None) |
        (std::is_void<base>::value ? Flags::Void : Flags::None) |
        (std::is_integral<base>::value ? Flags::Integral : Flags::None) |
        (std::is_floating_point<base>::value ? Flags::FloatPoint : Flags::None) |
        (std::is_enum<base>::value ? Flags::Enum : Flags::None) |
        (std::is_function<base>::value ? Flags::Function : Flags::None) |
        (std::is_union<base>::value ? Flags::Union : Flags::None) |
        (std::is_class<base>::value ? Flags::Class : Flags::None) |
        (std::is_pod<base>::value ? Flags::Pod : Flags::None) |
        (std::is_abstract<base>::value ? Flags::Abstract : Flags::None) |
        (std::is_polymorphic<base>::value ? Flags::Polymorphic : Flags::None) |
        (std::is_default_constructible<base>::value ? Flags::DefaultConstructible : Flags::None) |
        (std::is_copy_constructible<base>::value ? Flags::CopyConstructible : Flags::None) |
        (std::is_copy_assignable<base>::value ? Flags::CopyAssignable : Flags::None) |
        (std::is_move_constructible<base>::value ? Flags::MoveConstructible : Flags::None) |
        (std::is_move_assignable<base>::value ? Flags::MoveAssignable : Flags::None) |
        (std::is_destructible<base>::value ? Flags::Destructible : Flags::None)
    );
};

template <typename T>
class meta_type final
{
    using Decay = full_decay_t<T>;
    using NoRef = remove_reference_t<T>;
    using U = remove_all_cv_t<NoRef>;

    meta_type()
    {
        //register decayed type
        auto decay = decay_metatype(std::is_same<T, Decay>{});

        auto const &name = mpl::type_name<T>();
        auto const flags = type_flags<T>::value;
        auto const size = sizeof(T);
        std::uint16_t const arity = pointer_arity<NoRef>::value;
        std::uint16_t const const_mask = const_bitset<NoRef>::value;
        auto *manager = type_function_table_for<U>();
        meta_id = MetaType::registerMetaType(name.c_str(), size, decay,
                                             arity, const_mask, flags,
                                             manager);
    }

    static MetaType_ID decay_metatype(std::false_type)
    { return metaTypeId<Decay>(); }

    static MetaType_ID decay_metatype(std::true_type)
    { return MetaType_ID{}; }

    MetaType_ID meta_id;

    template <typename> friend MetaType_ID rtti::metaTypeId();
};

} // namespace internal

template <typename T>
inline MetaType_ID metaTypeId()
{
    static internal::meta_type<T> holder{};
    return holder.meta_id;
}

//--------------------------------------------------------------------------------------------------------------------------------
// Traits
//--------------------------------------------------------------------------------------------------------------------------------

inline bool MetaType::isConst() const noexcept
{
    return ((typeFlags() & Const) == Const);
}

inline bool MetaType::isLvalueReference() const noexcept
{
    return ((typeFlags() & LvalueReference) == LvalueReference);
}

inline bool MetaType::isRvalueReference() const noexcept
{
    return ((typeFlags() & RvalueReference) == RvalueReference);
}

inline bool MetaType::isReference() const noexcept
{
    return (isLvalueReference() || isRvalueReference());
}

inline bool MetaType::isClass() const noexcept
{
    auto flags = typeFlags();
    return ((flags & Class) == Class) &&
           ((flags & Pointer) == None);
}

inline bool MetaType::isPointer() const noexcept
{
    return ((typeFlags() & Pointer) == Pointer);
}

inline bool MetaType::isClassPtr() const noexcept
{
    auto flags = typeFlags();
    return (pointerArity() == 1) &&
           ((flags & Class) == Class) &&
            ((flags & Pointer) == Pointer);
}

inline bool MetaType::isArray() const noexcept
{
    auto flags = typeFlags();
    return ((flags & Array) == Array) &&
           ((flags & Pointer) == None);
}

//--------------------------------------------------------------------------------------------------------------------------------
// Converters
//--------------------------------------------------------------------------------------------------------------------------------

namespace internal {

template<typename From, typename To>
To default_convert(From value)
{
    return value;
}

struct DLL_LOCAL ConvertFunctionBase
{
    using converter_t = bool(*)(ConvertFunctionBase const&, void const*, void*);

    ConvertFunctionBase() = delete;
    ConvertFunctionBase(ConvertFunctionBase const&) = delete;
    ConvertFunctionBase(ConvertFunctionBase&&) = delete;
    ConvertFunctionBase& operator=(ConvertFunctionBase const&) = delete;
    ConvertFunctionBase& operator=(ConvertFunctionBase&&) = delete;

    explicit ConvertFunctionBase(converter_t converter)
        : m_converter(converter)
    {}

    bool invoke(void const *in, void *out) const
    {
        return m_converter(*this, in, out);
    }
private:
    converter_t m_converter;
};

template<typename From, typename To, typename F>
struct DLL_LOCAL ConvertFunctor: ConvertFunctionBase
{
    using this_t = ConvertFunctor<From, To, F>;

    explicit ConvertFunctor(F const &func)
        : ConvertFunctionBase{convert}, m_func(func)
    {}
    explicit ConvertFunctor(F &&func)
        : ConvertFunctionBase{convert}, m_func(std::move(func))
    {}
    ~ConvertFunctor()
    {
        MetaType::unregisterConverter<From, To>();
    }

    static bool convert(ConvertFunctionBase const &self, void const *in, void *out)
    {
        auto &_this = static_cast<this_t const&>(self);
        auto from = static_cast<From const*>(in);
        new (out) To(_this.m_func(*from));
        return true;
    }

private:
    F const m_func;
};

template<typename From, typename To>
struct DLL_LOCAL ConvertMethod: ConvertFunctionBase
{
    using this_t = ConvertMethod<From, To>;
    using func_t = To(From::*)() const;

    explicit ConvertMethod(func_t func)
        : ConvertFunctionBase{convert}, m_func(func)
    {}

    ~ConvertMethod()
    {
        MetaType::unregisterConverter<From, To>();
    }

    static bool convert(ConvertFunctionBase const &self, void const *in, void *out)
    {
        auto &_this = static_cast<this_t const&>(self);
        auto from = static_cast<From const*>(in);
        new (out) To(from->*_this.m_func());
        return true;
    }

private:
    func_t const m_func;
};

template<typename From, typename To>
struct DLL_LOCAL ConvertMethodOk: ConvertFunctionBase
{
    using this_t = ConvertMethodOk<From, To>;
    using func_t = To(From::*)(bool*) const;

    explicit ConvertMethodOk(func_t func)
        : ConvertFunctionBase{convert}, m_func(func)
    {}

    ~ConvertMethodOk()
    {
        MetaType::unregisterConverter<From, To>();
    }

    static bool convert(ConvertFunctionBase const &self, void const *in, void *out)
    {
        auto &_this = static_cast<this_t const&>(self);
        auto from = static_cast<From const*>(in);
        auto result = false;
        new (out) To(from->*_this.m_func(&result));
        return result;
    }

private:
    func_t const m_func;
};

} // namespace internal


template<typename From, typename To>
inline bool MetaType::hasConverter() noexcept
{
    return hasConverter(metaTypeId<From>(), metaTypeId<To>());
}

template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter_imp(Func &&func)
{
    static internal::ConvertFunctor<From, To, Func> converter{std::forward<Func>(func)};
    return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
}

template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter(Func &&func)
{
    using F = full_decay_t<From>;
    using T = full_decay_t<To>;
    using Fu = full_decay_t<Func>;
    return registerConverter_imp<F, T>(std::forward<Fu>(func));
}

template<typename From, typename To>
inline bool MetaType::registerConverter(To(*func)(From))
{
    using Func = To(*)(From);
    return registerConverter<From, To, Func>(std::move(func));
}

template<typename From, typename To>
inline bool MetaType::registerConverter()
{
    return registerConverter<From, To>(internal::default_convert<From, To>);
}

template<typename From, typename To>
inline bool MetaType::registerConverter_imp(To(From::*func)() const)
{
    static internal::ConvertMethod<From, To> converter{func};
    return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
}

template<typename From, typename To>
inline bool MetaType::registerConverter(To(From::*func)() const)
{
    using F = full_decay_t<From>;
    using T = full_decay_t<To>;
    return registerConverter_imp<F, T>(func);
}

template<typename From, typename To>
inline bool MetaType::registerConverter_imp(To(From::*func)(bool*) const)
{
    static internal::ConvertMethodOk<From, To> converter{func};
    return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
}

template<typename From, typename To>
inline bool MetaType::registerConverter(To(From::*func)(bool*) const)
{
    using F = full_decay_t<From>;
    using T = full_decay_t<To>;
    return registerConverter_imp<F, T>(func);
}

template<typename From, typename To>
inline void MetaType::unregisterConverter()
{
    unregisterConverter(metaTypeId<From>(), metaTypeId<To>());
}

//--------------------------------------------------------------------------------------------------------------------------------
// FUNDAMENTALS
//--------------------------------------------------------------------------------------------------------------------------------

template<> inline constexpr MetaType_ID metaTypeId<void>()
{ return MetaType_ID{0}; }

#define FOR_EACH_FUNDAMENTAL_TYPE(F)\
    F(bool, 1) \
    F(char, 2) \
    F(signed char, 3) \
    F(unsigned char, 4) \
    F(short, 5) \
    F(unsigned short, 6) \
    F(int, 7) \
    F(unsigned int, 8) \
    F(long int, 9) \
    F(long unsigned int, 10) \
    F(long long int, 11) \
    F(long long unsigned int, 12) \
    F(float, 13) \
    F(double, 14) \
    F(long double, 15) \
    F(char16_t, 16) \
    F(char32_t, 17) \
    F(wchar_t, 18) \
    F(void*, 19) \
    F(bool*, 20) \
    F(char*, 21) \
    F(signed char*, 22) \
    F(unsigned char*, 23) \
    F(short*, 24) \
    F(unsigned short*, 25) \
    F(int*, 26) \
    F(unsigned int*, 27) \
    F(long int*, 28) \
    F(long unsigned int*, 29) \
    F(long long int*, 30) \
    F(long long unsigned int*, 31) \
    F(float*, 32) \
    F(double*, 33) \
    F(long double*, 34) \
    F(char16_t*, 35) \
    F(char32_t*, 36) \
    F(wchar_t*, 37) \

#define DEFINE_STATIC_METATYPE_ID(NAME, TYPEID) \
template<> inline constexpr MetaType_ID metaTypeId<NAME>() \
{ return MetaType_ID{TYPEID}; }

FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_METATYPE_ID)

#undef DEFINE_STATIC_METATYPE_ID

} //namespace rtti

DLL_PUBLIC std::ostream& operator<<(std::ostream &stream, rtti::MetaType const &value);

#endif // METATYPE_H
