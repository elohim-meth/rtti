#ifndef METATYPE_H
#define METATYPE_H

#include "misc_traits.h"

#include <typename.h>
#include <tagged_id.h>

#include <limits>

#include "global.h"

namespace rtti {

struct meta_type_tag {};
using MetaType_ID = ID<meta_type_tag, std::uint32_t,
                       std::numeric_limits<std::uint32_t>::max()>;

// begin forward
namespace internal {
template<typename T> class meta_type;
struct ConvertFunctionBase;
} // namespace internal

struct TypeInfo;
class MetaClass;
class variant;
// end forward


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

    MetaType() = default;
    explicit MetaType(MetaType_ID typeId);
    explicit MetaType(const char *name);

    bool valid() const
    {
        return m_typeInfo != nullptr;
    }
    MetaType_ID typeId() const;
    MetaType_ID decayId() const;
    void setTypeId(MetaType_ID typeId);
    const char* typeName() const;
    std::size_t typeSize() const;
    MetaType::TypeFlags typeFlags() const;

    bool isConst() const;
    bool isLvalueReference() const;
    bool isRvalueReference() const;
    bool isReference() const;
    bool isClass() const;
    bool isClassPtr() const;
    bool isArray() const;
    std::uint8_t pointerArity() const;
    static bool constCompatible(MetaType fromType, MetaType toType);

    static bool hasConverter(MetaType fromType, MetaType toType);
    static bool hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId);
    template<typename From, typename To>
    static bool hasConverter();

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
    static MetaType_ID registerMetaType(const char *name, std::size_t size,
                                        MetaType_ID decay, std::uint8_t arity, uint8_t const_mask,
                                        MetaType::TypeFlags flags);

    template<typename From, typename To, typename Func>
    static bool registerConverter_imp(Func &&func);
    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)() const);
    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)(bool*) const);
    static bool registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                  const internal::ConvertFunctionBase &converter);
    static bool convert(const void *from, MetaType fromType, void *to, MetaType toType);
    static bool convert(const void *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId);

    const TypeInfo *m_typeInfo = nullptr;

    template<typename> friend class internal::meta_type;
    friend class rtti::MetaClass;
    friend class rtti::variant;
};

//forward
template <typename T> MetaType_ID metaTypeId();

namespace internal {

template <typename T>
struct type_flags {
    using Flags = MetaType::TypeFlags;
    using no_ref = remove_reference_t<T>;
    using no_ptr = conditional_t<std::is_array<T>::value,
                                 remove_all_extents_t<T>,
                                 remove_all_pointers_t<T>>;
    static const Flags value = static_cast<Flags>(
        (std::is_const<no_ref>::value ? Flags::Const : Flags::None) |
        (std::is_pointer<no_ref>::value ? Flags::Pointer : Flags::None) |
        (std::is_member_pointer<no_ref>::value ? Flags::MemberPointer : Flags::None) |
        (std::is_lvalue_reference<T>::value ? Flags::LvalueReference : Flags::None) |
        (std::is_rvalue_reference<T>::value ? Flags::RvalueReference : Flags::None) |
        (std::is_array<no_ref>::value ? Flags::Array : Flags::None) |
        (std::is_void<no_ptr>::value ? Flags::Void : Flags::None) |
        (std::is_integral<no_ptr>::value ? Flags::Integral : Flags::None) |
        (std::is_floating_point<no_ptr>::value ? Flags::FloatPoint : Flags::None) |
        (std::is_enum<no_ptr>::value ? Flags::Enum : Flags::None) |
        (std::is_function<no_ptr>::value ? Flags::Function : Flags::None) |
        (std::is_union<no_ptr>::value ? Flags::Union : Flags::None) |
        (std::is_class<no_ptr>::value ? Flags::Class : Flags::None) |
        (std::is_pod<no_ptr>::value ? Flags::Pod : Flags::None) |
        (std::is_abstract<no_ptr>::value ? Flags::Abstract : Flags::None) |
        (std::is_polymorphic<no_ptr>::value ? Flags::Polymorphic : Flags::None) |
        (std::is_default_constructible<no_ptr>::value ? Flags::DefaultConstructible : Flags::None) |
        (std::is_copy_constructible<no_ptr>::value ? Flags::CopyConstructible : Flags::None) |
        (std::is_copy_assignable<no_ptr>::value ? Flags::CopyAssignable : Flags::None) |
        (std::is_move_constructible<no_ptr>::value ? Flags::MoveConstructible : Flags::None) |
        (std::is_move_assignable<no_ptr>::value ? Flags::MoveAssignable : Flags::None) |
        (std::is_destructible<no_ptr>::value ? Flags::Destructible : Flags::None)
    );
};

template <typename T>
class meta_type final
{
    using Decay = full_decay_t<T>;
    using NoRef = remove_reference_t<T>;

    meta_type()
    {
        //register decayed type
        auto decay = decay_selector(std::is_same<T, Decay>{});

        const auto &name = type_name<T>();
        const auto flags = type_flags<T>::value;
        const auto size = sizeof(T);
        const std::uint8_t arity = pointer_arity<Decay>::value;
        const std::uint8_t const_mask = const_bitset<NoRef>::value;
        meta_id = MetaType::registerMetaType(name.c_str(), size, decay,
                                             arity, const_mask, flags);
    }

    static MetaType_ID decay_selector(std::false_type)
    { return metaTypeId<Decay>(); }

    static MetaType_ID decay_selector(std::true_type)
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

inline bool MetaType::isConst() const
{
    return ((typeFlags() & Const) == Const);
}

inline bool MetaType::isLvalueReference() const
{
    return ((typeFlags() & LvalueReference) == LvalueReference);
}

inline bool MetaType::isRvalueReference() const
{
    return ((typeFlags() & RvalueReference) == RvalueReference);
}

inline bool MetaType::isReference() const
{
    return (isLvalueReference() || isRvalueReference());
}

inline bool MetaType::isClass() const
{
    auto flags = typeFlags();
    return ((flags & Class) == Class) &&
           ((flags & Pointer) == None);
}

inline bool MetaType::isClassPtr() const
{
    auto flags = typeFlags();
    return (pointerArity() == 1) &&
           ((flags & Class) == Class) &&
            ((flags & Pointer) == Pointer);
}

inline bool MetaType::isArray() const
{
    return ((typeFlags() & Array) == Array);
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

template<typename From, typename To>
To constructor_convert(From value)
{
    return To(value);
}

struct DLL_LOCAL ConvertFunctionBase
{
    using converter_t = bool(*)(const ConvertFunctionBase&, const void*, void*);

    ConvertFunctionBase() = delete;
    ConvertFunctionBase(const ConvertFunctionBase&) = delete;
    ConvertFunctionBase(ConvertFunctionBase&&) = delete;
    ConvertFunctionBase& operator=(const ConvertFunctionBase&) = delete;
    ConvertFunctionBase& operator=(ConvertFunctionBase&&) = delete;

    explicit ConvertFunctionBase(converter_t converter)
        : m_converter(converter)
    {}

    bool invoke(const void *in, void *out) const
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

    explicit ConvertFunctor(const F &func)
        : ConvertFunctionBase{convert}, m_func(func)
    {}
    explicit ConvertFunctor(F &&func)
        : ConvertFunctionBase{convert}, m_func(std::move(func))
    {}
    ~ConvertFunctor()
    {
        MetaType::unregisterConverter<From, To>();
    }

    static bool convert(const ConvertFunctionBase &self, const void *in, void *out)
    {
        auto &_this = static_cast<const this_t&>(self);
        auto from = static_cast<const From*>(in);
        new (out) To(_this.m_func(*from));
        return true;
    }

private:
    const F m_func;
};

template<typename From, typename To>
struct ConvertMethod: ConvertFunctionBase
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

    static bool convert(const ConvertFunctionBase &self, const void *in, void *out)
    {
        auto &_this = static_cast<const this_t&>(self);
        auto from = static_cast<const From*>(in);
        new (out) To(from->*_this.m_func());
        return true;
    }

private:
    const func_t m_func;
};

template<typename From, typename To>
struct ConvertMethodOk: ConvertFunctionBase
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

    static bool convert(const ConvertFunctionBase &self, const void *in, void *out)
    {
        auto &_this = static_cast<const this_t&>(self);
        auto from = static_cast<const From*>(in);
        auto result = false;
        new (out) To(from->*_this.m_func(&result));
        return result;
    }

private:
    const func_t m_func;
};

} // namespace internal


template<typename From, typename To>
inline bool MetaType::hasConverter()
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

DLL_PUBLIC std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value);

#endif // METATYPE_H
