#ifndef METATYPE_H
#define METATYPE_H

#include <rtti/defines.h>
#include <rtti/export.h>

#include <rtti/misc_traits.h>
#include <rtti/metaerror.h>

#include <rtti/typename.h>
#include <rtti/tagged_id.h>

namespace rtti {

namespace internal {

struct meta_type_tag {};

/* begin forward */
struct type_function_table;
template<typename T> struct type_function_table_impl;
template<typename T> class meta_type;
struct ConvertFunctionBase;

} // namespace internal

using metatype_manager_t = internal::type_function_table;
template<typename T>
using type_manager_t = internal::type_function_table_impl<remove_all_cv_t<std::remove_reference_t<T>>>;

struct TypeInfo;
class MetaClass;
class variant;
class argument;
/* end forward */

using MetaType_ID = mpl::ID<internal::meta_type_tag, std::size_t, 0>;

enum class TypeFlags: std::uint32_t {
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

    StandardLayout       = 1 << 13,
    Trivial              = 1 << 14,
    Abstract             = 1 << 15,
    Polymorphic          = 1 << 16,
    DefaultConstructible = 1 << 17,
    CopyConstructible    = 1 << 18,
    CopyAssignable       = 1 << 19,
    MoveConstructible    = 1 << 20,
    MoveAssignable       = 1 << 21,
    Destructible         = 1 << 22,

    EQ_Comparable        = 1 << 23,
};

BITMASK_ENUM(TypeFlags)
std::ostream& operator<<(std::ostream &stream, TypeFlags value);

class RTTI_API MetaType final {
public:
    MetaType() noexcept = default;
    explicit MetaType(MetaType_ID typeId) noexcept;
    explicit MetaType(std::string_view name) noexcept;

    bool valid() const noexcept
    {
        return (m_typeInfo != nullptr);
    }
    MetaType_ID typeId() const noexcept;
    MetaType_ID decayId() const noexcept;
    bool decayed() const noexcept
    { return valid() && (typeId() == decayId()); }
    std::string_view typeName() const noexcept;
    std::size_t typeSize() const noexcept;
    TypeFlags typeFlags() const noexcept;

    inline bool isConst() const noexcept;
    inline bool isLvalueReference() const noexcept;
    inline bool isRvalueReference() const noexcept;
    inline bool isReference() const noexcept;
    inline bool isClass() const noexcept;
    inline bool isPointer() const noexcept;
    inline bool isVoidPtr() const noexcept;
    inline bool isClassPtr() const noexcept;
    inline bool isFunctionPtr() const noexcept;
    inline bool isArray() const noexcept;

    uint16_t pointerArity() const noexcept;
    static bool compatible(MetaType fromType, MetaType toType) noexcept;

    MetaClass const* metaClass() const noexcept;

    void* construct(void *copy = nullptr, bool movable = false) const;
    void destruct(void *instance) const;

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
    static bool registerConverter(To(From::*func)(bool&) const);

    static void unregisterConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId);
    template<typename From, typename To>
    static void unregisterConverter();
private:
    static MetaType_ID registerMetaType(std::string_view name, std::size_t size,
        MetaType_ID decay, uint16_t arity, uint16_t const_mask,
        TypeFlags flags, metatype_manager_t const *manager);

    void* allocate() const;
    void deallocate(void *ptr) const;
    void default_construct(void *where) const;
    void copy_construct(void const *source, void *where) const;
    void move_construct(void *source, void *where) const;
    void move_or_copy(void *source, void *where) const;
    void destroy(void *ptr) const noexcept;
    bool compare_eq(void const *lhs, void const *rhs) const;

    template<typename From, typename To, typename Func>
    static bool registerConverter_imp(Func &&func);

    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)() const);

    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)(bool&) const);

    static bool registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                  internal::ConvertFunctionBase const &converter);
    static bool convert(void const *from, MetaType fromType, void *to, MetaType toType);
    static bool convert(void const *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId);

    TypeInfo const *m_typeInfo = nullptr;
private:
    DECLARE_ACCESS_KEY(TypeInfoKey)
        friend class rtti::MetaClass;
    };
    DECLARE_ACCESS_KEY(RegisterTypeKey)
        template<typename> friend class internal::meta_type;
    };
    friend class rtti::variant;
public:
    TypeInfo const* typeInfo(TypeInfoKey) const
    { return m_typeInfo; }
    static MetaType_ID registerMetaType(std::string_view name, std::size_t size,
                                        MetaType_ID decay, uint16_t arity, uint16_t const_mask,
                                        TypeFlags flags, metatype_manager_t const *manager,
                                        RegisterTypeKey)
    { return registerMetaType(name, size, decay, arity, const_mask, flags, manager); }
};

//forward
template <typename T> MetaType_ID metaTypeId();

namespace internal {

struct RTTI_PRIVATE type_function_table
{
    using allocate_t = void* (*) ();
    using deallocate_t = void (*) (void*);
    using default_construct_t = void (*) (void*);
    using copy_construct_t = void (*) (void const*, void*);
    using move_construct_t = void (*) (void*, void*);
    using move_or_copy_t = void (*) (void*, void*);
    using destroy_t = void (*) (void*);
    // comparators
    using compare_eq_t = bool (*) (void const*, void const*);

    allocate_t const f_allocate = nullptr;
    deallocate_t const f_deallocate = nullptr;
    default_construct_t const f_default_construct = nullptr;
    copy_construct_t const f_copy_construct = nullptr;
    move_construct_t const f_move_construct = nullptr;
    move_or_copy_t const f_move_or_copy = nullptr;
    destroy_t const f_destroy = nullptr;
    compare_eq_t const f_compare_eq = nullptr;

    constexpr type_function_table(allocate_t allocate, deallocate_t deallocate,
                                  default_construct_t default_construct,
                                  copy_construct_t copy_construct, move_construct_t move_construct,
                                  move_or_copy_t move_or_copy, destroy_t destroy,
                                  compare_eq_t compare_eq) noexcept
        : f_allocate{allocate}
        , f_deallocate{deallocate}
        , f_default_construct{default_construct}
        , f_copy_construct{copy_construct}
        , f_move_construct{move_construct}
        , f_move_or_copy{move_or_copy}
        , f_destroy{destroy}
        , f_compare_eq{compare_eq}
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

    DISABLE_WARNINGS_PUSH
    DISABLE_WARNING_INIT_LIST_LIFETIME

    static void default_construct([[maybe_unused]] void *where)
        noexcept(std::is_nothrow_default_constructible_v<T>)
    {
        using namespace std::literals;
        if constexpr(std::is_default_constructible_v<T>)
        {
            if constexpr(!std::is_trivially_default_constructible_v<T>)
                if (where)
                    new (where) T();
        }
        else throw runtime_error("Type T = "s + type_name<T>() + "isn't DefaultConstructible");
    }

    static void copy_construct(void const *source, void *where)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        using namespace std::literals;
        if constexpr(std::is_copy_constructible_v<T>)
        {
            if (source && where)
                new (where) T(*static_cast<T const*>(source));
        }
        else throw runtime_error("Type T = "s + type_name<T>() + "isn't CopyConstructible");

    }

    static void move_construct(void *source, void *where)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        using namespace std::literals;
        if constexpr(std::is_move_constructible_v<T>)
        {
            if (source && where)
                new (where) T(std::move(*static_cast<T*>(source)));
        }
        else throw runtime_error("Type T = "s + type_name<T>() + "isn't MoveConstructible");
    }

    DISABLE_WARNINGS_POP

    static void move_or_copy(void *source, void *where)
    {
        if constexpr(std::is_move_constructible_v<T>)
            move_construct(source, where);
        else
            copy_construct(source, where);
    }

    static void destroy([[maybe_unused]] void *ptr) noexcept
    {
        if constexpr(!std::is_trivially_destructible_v<T>)
            if (ptr)
                static_cast<T*>(ptr)->~T();
    }

    static bool compare_eq(void const *lhs, void const *rhs)
        noexcept(has_nt_eq_v<T, T>)
    {
        using namespace std::literals;
        if constexpr(has_eq_v<T, T>)
        {
            if (lhs == rhs)
                return true;
            if (!lhs || !rhs)
                return false;
            return (*static_cast<T const*>(lhs) == *static_cast<T const *>(rhs));
        }
        else throw runtime_error("Type T = "s + type_name<T>() + "isn't EQ_comparable");
    }
};

template<typename T, std::size_t N>
struct type_function_table_impl<T[N]>
{
    using Base = std::remove_all_extents_t<T[N]>;
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

    static void default_construct([[maybe_unused]] void *where)
        noexcept(std::is_nothrow_default_constructible<Base>::value)
    {
        using namespace std::literals;
        if constexpr(std::is_default_constructible_v<Base>)
        {
            if constexpr(!std::is_trivially_default_constructible_v<Base>)
                if (where)
                {
                    auto *begin = static_cast<Base*>(where);
                    auto *end = static_cast<Base*>(where) + Length;

                    for (; begin != end; ++begin)
                        new (begin) Base();
                }
        }
        else throw runtime_error("Type T = "s + type_name<Base>() + "isn't DefaultConstructible");
    }

    static void copy_construct(void const *source, void *where)
        noexcept(std::is_nothrow_copy_constructible<Base>::value)
    {
        using namespace std::literals;
        if constexpr(std::is_copy_constructible_v<Base>)
        {
            if (source && where)
            {
                auto *it = static_cast<Base const*>(source);
                std::copy(it, it + Length, static_cast<Base*>(where));
            }
        }
        else throw runtime_error("Type T = "s + type_name<Base>() + "isn't CopyConstructible");
    }

    static void move_construct(void *source, void *where)
        noexcept(std::is_nothrow_move_constructible<Base>::value)
    {
        using namespace std::literals;
        if constexpr(std::is_move_constructible_v<Base>)
        {
            if (source && where)
            {
                auto *it = static_cast<Base*>(source);
                std::move(it, it + Length, static_cast<Base*>(where));
            }
        }
        else throw runtime_error("Type T = "s + type_name<Base>() + "isn't MoveConstructible");
    }

    static void move_or_copy(void *source, void *where)
    {
        if constexpr(std::is_move_constructible_v<Base>)
            move_construct(source, where);
        else
            copy_construct(source, where);
    }

    static void destroy([[maybe_unused]] void *ptr) noexcept
    {
        if constexpr(!std::is_trivially_destructible_v<Base>)
            if (ptr)
            {
                auto *begin = static_cast<Base*>(ptr);
                auto *end = static_cast<Base*>(ptr) + Length;

                for (; begin != end; ++begin)
                    begin->~Base();
            }
    }

    static bool compare_eq(void const *lhs, void const *rhs)
        noexcept(has_nt_eq_v<T, T>)
    {
        using namespace std::literals;
        if constexpr(has_eq_v<T, T>)
        {
            if (lhs == rhs)
                return true;
            if (!lhs || !rhs)
                return false;

            auto *first1 = static_cast<Base const*>(lhs);
            auto *last1 = static_cast<Base const*>(lhs) + Length;
            auto *first2 = static_cast<Base const*>(rhs);
            return std::equal(first1, last1, first2);
        }
        else throw runtime_error("Type T = "s + type_name<T>() + "isn't EQ_Comparable");
    }

};

template <typename T>
inline T move_or_copy(void *source)
{
    using namespace std::literals;
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,
                  "Type should be CopyConstructible or MoveConstructible");

    if constexpr(std::is_move_constructible_v<T>)
        return std::move(*static_cast<T*>(source));

    if constexpr(std::is_copy_constructible_v<T>)
         return *static_cast<T*>(source);

    throw runtime_error("Type T = "s + type_name<T>() + "isn't CopyConstructible");
}

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
        &type_function_table_impl<T>::destroy,
        &type_function_table_impl<T>::compare_eq
    };
    return &result;
}

template<>
inline type_function_table const* type_function_table_for<void>() noexcept
{ return nullptr; }

template <typename T>
struct type_flags {
    using Flags = TypeFlags;
    using no_ref = std::remove_reference_t<T>;
    using no_ptr = std::remove_pointer_t<no_ref>;
    using base = base_type_t<T>;
    static Flags constexpr value =
          (std::is_const_v<no_ref>                 ? Flags::Const                  : Flags::None)
        | (std::is_pointer_v<no_ref>               ? Flags::Pointer                : Flags::None)
        | (std::is_member_pointer_v<no_ref>        ? Flags::MemberPointer          : Flags::None)
        | (std::is_lvalue_reference_v<T>           ? Flags::LvalueReference        : Flags::None)
        | (std::is_rvalue_reference_v<T>           ? Flags::RvalueReference        : Flags::None)
        | (std::is_array_v<no_ptr>                 ? Flags::Array                  : Flags::None)
        | (std::is_void_v<base>                    ? Flags::Void                   : Flags::None)
        | (std::is_integral_v<base>                ? Flags::Integral               : Flags::None)
        | (std::is_floating_point_v<base>          ? Flags::FloatPoint             : Flags::None)
        | (std::is_enum_v<base>                    ? Flags::Enum                   : Flags::None)
        | (std::is_function_v<base>                ? Flags::Function               : Flags::None)
        | (std::is_union_v<base>                   ? Flags::Union                  : Flags::None)
        | (std::is_class_v<base>                   ? Flags::Class                  : Flags::None)
        | (std::is_standard_layout_v<base>         ? Flags::StandardLayout         : Flags::None)
        | (std::is_trivial_v<base>                 ? Flags::Trivial                : Flags::None)
        | (std::is_abstract_v<base>                ? Flags::Abstract               : Flags::None)
        | (std::is_polymorphic_v<base>             ? Flags::Polymorphic            : Flags::None)
        | (std::is_default_constructible_v<base>   ? Flags::DefaultConstructible   : Flags::None)
        | (std::is_copy_constructible_v<base>      ? Flags::CopyConstructible      : Flags::None)
        | (std::is_copy_assignable_v<base>         ? Flags::CopyAssignable         : Flags::None)
        | (std::is_move_constructible_v<base>      ? Flags::MoveConstructible      : Flags::None)
        | (std::is_move_assignable_v<base>         ? Flags::MoveAssignable         : Flags::None)
        | (std::is_destructible_v<base>            ? Flags::Destructible           : Flags::None)
        | (has_eq_v<no_ref,no_ref>                 ? Flags::EQ_Comparable          : Flags::None)
    ;
};

template <typename T>
class meta_type final
{
    using Decay = full_decay_t<T>;
    using NoRef = std::remove_reference_t<T>;
    using U = remove_all_cv_t<NoRef>;

    meta_type()
    {
        //register decayed type
        auto decay = MetaType_ID{};
        if constexpr(!std::is_same_v<T, Decay>)
            decay = metaTypeId<Decay>();

        auto const &name = type_name<T>();
        auto constexpr flags = type_flags<T>::value;
        auto constexpr size = size_of_v<T>;
        std::uint16_t constexpr arity = pointer_arity<NoRef>::value;
        std::uint16_t constexpr const_mask = const_bitset<NoRef>::value;
        auto *manager = type_function_table_for<U>();
        meta_id = MetaType::registerMetaType(name, size, decay,
                                             arity, const_mask, flags,
                                             manager, {});
    }

    MetaType_ID meta_id;

    friend MetaType_ID rtti::metaTypeId<T>();
};

} // namespace internal

template <typename T>
inline MetaType_ID metaTypeId()
{
    static internal::meta_type<T> holder{};
    return holder.meta_id;
}

template<>
inline MetaType_ID metaTypeId<void>()
{ return MetaType_ID{}; }

template <typename T>
inline MetaType metaType()
{ return MetaType{metaTypeId<T>()}; }

template <typename T>
inline MetaType metaType(T&&)
{ return MetaType{metaTypeId<T>()}; }


//--------------------------------------------------------------------------------------------------------------------------------
// Traits
//--------------------------------------------------------------------------------------------------------------------------------

inline bool MetaType::isConst() const noexcept
{
    return ((typeFlags() & TypeFlags::Const) == TypeFlags::Const);
}

inline bool MetaType::isLvalueReference() const noexcept
{
    return ((typeFlags() & TypeFlags::LvalueReference) == TypeFlags::LvalueReference);
}

inline bool MetaType::isRvalueReference() const noexcept
{
    return ((typeFlags() & TypeFlags::RvalueReference) == TypeFlags::RvalueReference);
}

inline bool MetaType::isReference() const noexcept
{
    return (isLvalueReference() || isRvalueReference());
}

inline bool MetaType::isClass() const noexcept
{
    auto flags = typeFlags();
    return ((flags & TypeFlags::Class) == TypeFlags::Class) &&
           ((flags & TypeFlags::Pointer) == TypeFlags::None);
}

inline bool MetaType::isPointer() const noexcept
{
    return ((typeFlags() & TypeFlags::Pointer) == TypeFlags::Pointer);
}

inline bool MetaType::isVoidPtr() const noexcept
{
    auto flags = typeFlags();
    return ((flags & TypeFlags::Void) == TypeFlags::Void) &&
           ((flags & TypeFlags::Pointer) == TypeFlags::Pointer);
}

inline bool MetaType::isClassPtr() const noexcept
{
    auto flags = typeFlags();
    return (pointerArity() == 1) &&
           ((flags & TypeFlags::Class) == TypeFlags::Class) &&
            ((flags & TypeFlags::Pointer) == TypeFlags::Pointer);
}

inline bool MetaType::isFunctionPtr() const noexcept
{
    auto flags = typeFlags();
    return (pointerArity() == 1) &&
           ((flags & TypeFlags::Function) == TypeFlags::Function) &&
            ((flags & TypeFlags::Pointer) == TypeFlags::Pointer);
}

inline bool MetaType::isArray() const noexcept
{
    auto flags = typeFlags();
    return ((flags & TypeFlags::Array) == TypeFlags::Array) &&
           ((flags & TypeFlags::Pointer) == TypeFlags::None);
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

struct RTTI_PRIVATE ConvertFunctionBase
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
struct RTTI_PRIVATE ConvertFunctor: ConvertFunctionBase
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
        try
        {
            auto &_this = static_cast<this_t const&>(self);
            auto from = static_cast<From const*>(in);
            new (out) To(_this.m_func(*from));
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

private:
    F const m_func;
};

template<typename From, typename To, typename F>
struct RTTI_PRIVATE ConvertFunctorOk: ConvertFunctionBase
{
    using this_t = ConvertFunctorOk<From, To, F>;

    explicit ConvertFunctorOk(F const &func)
        : ConvertFunctionBase{convert}, m_func(func)
    {}
    explicit ConvertFunctorOk(F &&func)
        : ConvertFunctionBase{convert}, m_func(std::move(func))
    {}
    ~ConvertFunctorOk()
    {
        MetaType::unregisterConverter<From, To>();
    }

    static bool convert(ConvertFunctionBase const &self, void const *in, void *out)
    {
        auto &_this = static_cast<this_t const&>(self);
        auto from = static_cast<From const*>(in);
        auto result = false;
        new (out) To(_this.m_func(*from, result));
        return result;
    }

private:
    F const m_func;
};

template<typename From, typename To>
struct RTTI_PRIVATE ConvertMethod: ConvertFunctionBase
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
        try
        {
            auto &_this = static_cast<this_t const&>(self);
            auto from = static_cast<From const*>(in);
            new (out) To(from->*_this.m_func());
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

private:
    func_t const m_func;
};

template<typename From, typename To>
struct RTTI_PRIVATE ConvertMethodOk: ConvertFunctionBase
{
    using this_t = ConvertMethodOk<From, To>;
    using func_t = To(From::*)(bool&) const;

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
        new (out) To(from->*_this.m_func(result));
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

// Functor converter

template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter_imp(Func &&func)
{
    using Fu = full_decay_t<Func>;
    auto constexpr is_functorOk = std::is_invocable_r_v<To, Func, From, bool&>;

    if constexpr(is_functorOk)
    {
        static internal::ConvertFunctorOk<From, To, Fu> converter{std::forward<Func>(func)};
        return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
    }
    else
    {
        static internal::ConvertFunctor<From, To, Fu> converter{std::forward<Func>(func)};
        return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
    }
}

template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter(Func &&func)
{
    using F = full_decay_t<From>;
    using T = full_decay_t<To>;
    return registerConverter_imp<F, T>(std::forward<Func>(func));
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

// Method converter

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

// Method extended converter

template<typename From, typename To>
inline bool MetaType::registerConverter_imp(To(From::*func)(bool&) const)
{
    static internal::ConvertMethodOk<From, To> converter{func};
    return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
}

template<typename From, typename To>
inline bool MetaType::registerConverter(To(From::*func)(bool&) const)
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

RTTI_API std::ostream& operator<<(std::ostream &stream, MetaType value);

} //namespace rtti


#endif // METATYPE_H
