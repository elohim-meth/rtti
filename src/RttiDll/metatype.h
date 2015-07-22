#ifndef METATYPE_H
#define METATYPE_H

#include <typename.h>
#include <typelist.h>
#include <tagged_id.h>

#include <atomic>
#include <type_traits>
#include <limits>

#include "global.h"

namespace rtti {

namespace internal {

template <typename T>
struct remove_member_pointer
{
    using type = T;
};
template <typename T, typename C>
struct remove_member_pointer<T C::*>
{
    using type = T;
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...)>
{
    using type = R (C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) &>
{
    using type = R (C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) &&>
{
    using type = R (C&&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const>
{
    using type = R (const C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const &>
{
    using type = R (const C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const &&>
{
    using type = R (const C&&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) volatile>
{
    using type = R (volatile C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) volatile &>
{
    using type = R (volatile C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) volatile &&>
{
    using type = R (volatile C&&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const volatile>
{
    using type = R (const volatile C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const volatile &>
{
    using type = R (const volatile C&, Args...);
};
template <typename R, typename C, typename ...Args>
struct remove_member_pointer<R (C::*)(Args...) const volatile &&>
{
    using type = R (const volatile C&&, Args...);
};

template<typename T>
using decay_t = typename std::decay<T>::type;

template<typename T, bool = std::is_pointer<T>::value>
struct pointer_count;

template<typename T>
struct pointer_count<T, false>: std::integral_constant<std::size_t, 0>
{};

template<typename T>
struct pointer_count<T, true>: std::integral_constant<std::size_t,
                                                      pointer_count<typename std::remove_pointer<T>::type>::value + 1>
{};

template<typename T, bool = std::is_pointer<T>::value>
struct remove_all_pointers;

template<typename T>
struct remove_all_pointers<T, false>: identity<typename std::remove_cv<T>::type>
{};

template<typename T>
struct remove_all_pointers<T, true>:
    remove_all_pointers<
        typename std::remove_cv<
            typename std::remove_pointer<T>::type>
        ::type>
{};

template<typename T>
using remove_all_pointers_t = typename remove_all_pointers<T>::type;

template<typename T, std::size_t I>
struct add_pointers: add_pointers<typename std::add_pointer<T>::type, I - 1>
{};

template<typename T>
struct add_pointers<T, 0>: identity<T>
{};

template<typename T, std::size_t I>
using add_pointers_t = typename add_pointers<T, I>::type;

template<typename T>
struct full_decay: add_pointers<remove_all_pointers_t<decay_t<T>>, pointer_count<decay_t<T>>::value>
{};

template<typename T>
using full_decay_t = typename full_decay<T>::type;

template<typename T>
using is_polymorphic_ptr =
typename std::conditional<
    std::is_pointer<typename std::remove_reference<T>::type>::value &&
    std::is_polymorphic<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::value,
    std::true_type, std::false_type
>::type;

template<typename T>
using is_class_ptr =
typename std::conditional<
    std::is_pointer<typename std::remove_reference<T>::type>::value &&
    std::is_class<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::value,
    std::true_type, std::false_type
>::type;

template<typename T, typename ...Args>
struct is_converting_constructor: std::false_type
{};

template<typename T>
struct is_converting_constructor<T>: std::false_type
{};

template<typename T, typename Arg>
struct is_converting_constructor<T, Arg>:
    std::conditional<!std::is_same<T, internal::full_decay_t<Arg>>::value,
                     std::true_type, std::false_type>::type
{};

} // namespace internal

struct meta_type_tag {};
using MetaType_ID = ID<meta_type_tag, uint32_t,
                       std::numeric_limits<uint32_t>::max()>;


// begin forward
namespace internal {
template<typename T> struct meta_type;
class ConvertFunctionBase;
} // namespace internal

class TypeInfo;
class MetaClass;
class variant;
template <typename T> MetaType_ID metaTypeId();
// end forward


class DLL_PUBLIC MetaType final {
public:
    enum : MetaType_ID::type {
        InvalidTypeId = MetaType_ID::Default
    };

    enum TypeFlags: unsigned int {
        None                 = 0,

        Const                = 1 << 0,
        Volatile             = 1 << 1,
        Pointer              = 1 << 2,
        MemberPointer        = 1 << 3,
        LvalueReference      = 1 << 4,
        RvalueReference      = 1 << 5,
        Array                = 1 << 6,

        Void                 = 1 << 7,
        Integral             = 1 << 8,
        FloatPoint           = 1 << 9,
        Enum                 = 1 << 10,
        Function             = 1 << 11,
        Union                = 1 << 12,
        Class                = 1 << 13,

        Pod                  = 1 << 14,
        Abstract             = 1 << 15,
        Polymorphic          = 1 << 16,
        DefaultConstructible = 1 << 17,
        CopyConstructible    = 1 << 18,
        CopyAssignable       = 1 << 19,
        MoveConstructible    = 1 << 20,
        MoveAssignable       = 1 << 21,
        Destructible         = 1 << 22,

        ClassPtr             = 1 << 23
    };

    MetaType() noexcept = default;
    explicit MetaType(MetaType_ID typeId);
    explicit MetaType(const char *name);

    bool valid() const noexcept
    {
        return m_typeInfo != nullptr;
    }
    MetaType_ID typeId() const noexcept;
    void setTypeId(MetaType_ID typeId);
    const char* typeName() const noexcept;
    std::size_t typeSize() const noexcept;
    MetaType::TypeFlags typeFlags() const noexcept;


    static bool hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId);
    template<typename From, typename To>
    static bool hasConverter()
    {
        return hasConverter(metaTypeId<From>(), metaTypeId<To>());
    }

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
    static void unregisterConverter()
    {
        unregisterConverter(metaTypeId<From>(), metaTypeId<To>());
    }
private:
    static MetaType_ID registerMetaType(const char *name, unsigned int size,
                                        MetaType_ID decay,
                                        MetaType::TypeFlags flags);

    template<typename From, typename To, typename Func>
    static bool registerConverter_imp(Func &&func);
    template<typename From, typename To>
    static bool registerConverter_imp(To(From::*func)() const);
    template<typename From, typename To>
    bool registerConverter_imp(To(From::*func)(bool*) const);
    static bool registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                  const internal::ConvertFunctionBase &converter);
    static bool convert(const void *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId);

    const TypeInfo *m_typeInfo = nullptr;

    template<typename T>
    friend struct internal::meta_type;
    friend class rtti::MetaClass;
    friend class rtti::variant;
};

template <typename T>
struct type_flags {
    using Flags = MetaType::TypeFlags;
    static const Flags value = static_cast<Flags>(
        (std::is_const<T>::value ? Flags::Const : Flags::None) |
        (std::is_volatile<T>::value ? Flags::Volatile : Flags::None) |
        (std::is_pointer<T>::value ? Flags::Pointer : Flags::None) |
        (std::is_member_pointer<T>::value ? Flags::MemberPointer : Flags::None) |
        (std::is_lvalue_reference<T>::value ? Flags::LvalueReference : Flags::None) |
        (std::is_rvalue_reference<T>::value ? Flags::RvalueReference : Flags::None) |
        (std::is_array<T>::value ? Flags::Array : Flags::None) |
        (std::is_void<T>::value ? Flags::Void : Flags::None) |
        (std::is_integral<T>::value ? Flags::Integral : Flags::None) |
        (std::is_floating_point<T>::value ? Flags::FloatPoint : Flags::None) |
        (std::is_enum<T>::value ? Flags::Enum : Flags::None) |
        (std::is_function<T>::value ? Flags::Function : Flags::None) |
        (std::is_union<T>::value ? Flags::Union : Flags::None) |
        (std::is_class<T>::value ? Flags::Class : Flags::None) |
        (std::is_pod<T>::value ? Flags::Pod : Flags::None) |
        (std::is_abstract<T>::value ? Flags::Abstract : Flags::None) |
        (std::is_polymorphic<T>::value ? Flags::Polymorphic : Flags::None) |
        (std::is_default_constructible<T>::value ? Flags::DefaultConstructible : Flags::None) |
        (std::is_copy_constructible<T>::value ? Flags::CopyConstructible : Flags::None) |
        (std::is_copy_assignable<T>::value ? Flags::CopyAssignable : Flags::None) |
        (std::is_move_constructible<T>::value ? Flags::MoveConstructible : Flags::None) |
        (std::is_move_assignable<T>::value ? Flags::MoveAssignable : Flags::None) |
        (std::is_destructible<T>::value ? Flags::Destructible : Flags::None) |
        (internal::is_class_ptr<T>::value ? Flags::ClassPtr : Flags::None)
    );
};

namespace internal {

template <typename T>
struct meta_type
{
    using decayed_t = full_decay_t<T>;

    static MetaType_ID get()
    {
        if (const auto id = meta_id.load())
            return MetaType_ID{id};

        auto decay = decay_selector(std::is_same<T, decayed_t>{});

        auto &name = type_name<T>();
        const auto flags = type_flags<T>::value;
        const auto id = MetaType::registerMetaType(name.c_str(), sizeof(T), decay, flags);
        meta_id.store(id.value());
        return id;
    }

    static MetaType_ID decay_selector(std::false_type)
    {
        return metaTypeId<decayed_t>();
    }

    static MetaType_ID decay_selector(std::true_type)
    {
        return MetaType_ID{};
    }

    // declaration
    static std::atomic<MetaType_ID::type> meta_id;
};

// definition
template<typename T>
std::atomic<MetaType_ID::type> meta_type<T>::meta_id = {0};

} // namespace internal

template <typename T>
inline MetaType_ID metaTypeId()
{
    return internal::meta_type<T>::get();
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
    ~ConvertFunctor() noexcept
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

    ~ConvertMethod() noexcept
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

    ~ConvertMethodOk() noexcept
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


template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter_imp(Func &&func)
{
    static internal::ConvertFunctor<From, To, Func> converter{std::forward<Func>(func)};
    return registerConverter(metaTypeId<From>(), metaTypeId<To>(), converter);
}

template<typename From, typename To, typename Func>
inline bool MetaType::registerConverter(Func &&func)
{
    using F = internal::full_decay_t<From>;
    using T = internal::full_decay_t<To>;
    using Fu = internal::full_decay_t<Func>;
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
    using F = internal::full_decay_t<From>;
    using T = internal::full_decay_t<To>;
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
    using F = internal::full_decay_t<From>;
    using T = internal::full_decay_t<To>;
    return registerConverter_imp<F, T>(func);
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
    F(void const*, 38) \
    F(char const*, 39) \
    F(wchar_t const*, 40) \

#define DEFINE_STATIC_METATYPE_ID(NAME, TYPEID) \
template<> inline constexpr MetaType_ID metaTypeId<NAME>() \
{ return MetaType_ID{TYPEID}; }

FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_METATYPE_ID)

#undef DEFINE_STATIC_METATYPE_ID

} //namespace rtti

DLL_PUBLIC std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value);

#endif // METATYPE_H
