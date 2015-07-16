﻿#ifndef METATYPE_H
#define METATYPE_H

#include <typename.h>
#include <tagged_id.h>

#include <atomic>
#include <type_traits>
#include <limits>

#include "global.h"

namespace rtti {

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

template<typename T,
         bool Const = std::is_const<T>::value,
         bool Volatile = std::is_volatile<T>::value,
         bool Array = std::is_array<T>::value,
         bool Reference = std::is_reference<T>::value,
         bool Pointer = std::is_pointer<T>::value,
         bool MemberPtr = std::is_member_pointer<T>::value>
struct base_type {
    using type =
        typename std::conditional<Const, typename std::remove_cv<T>::type,
        typename std::conditional<Volatile, typename std::remove_cv<T>::type,
        typename std::conditional<Reference, typename std::remove_reference<T>::type,
        typename std::conditional<Array, typename std::remove_all_extents<T>::type,
        typename std::conditional<Pointer, typename std::remove_pointer<T>::type,
        typename std::conditional<MemberPtr, typename remove_member_pointer<T>::type,
        T>::type>::type>::type>::type>::type>::type;
};

struct meta_type_tag {};
using MetaType_ID = ID<meta_type_tag, uint32_t,
                       std::numeric_limits<uint32_t>::max()>;

// forward
class TypeInfo;
namespace internal {
template<typename T> struct meta_type;
}

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
    MetaType::TypeFlags typeFlags() const noexcept;

    template<typename From, typename To, typename F>
    static void registerConverter(F func);
    template<typename From, typename To>
    static void registerConverter();

private:
    template<typename T>
    friend struct internal::meta_type;
    friend class MetaClass;

    static MetaType_ID registerMetaType(const char *name, unsigned int size,
                                        MetaType_ID decay,
                                        MetaType::TypeFlags flags);

    const TypeInfo *m_typeInfo = nullptr;
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
        (std::is_destructible<T>::value ? Flags::Destructible : Flags::None));
};

// forward
template <typename T> MetaType_ID metaTypeId();

namespace internal {

template <typename T>
struct meta_type
{
    using decayed_t = typename std::decay<T>::type;

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
// Convertors
//--------------------------------------------------------------------------------------------------------------------------------

template<typename From, typename To>
To default_convert(From value)
{
    return value;
}

struct DLL_LOCAL ConvertFunctionBase
{
    using converter_t = bool (*) (const ConvertFunctionBase&, const void*, void*);

    explicit ConvertFunctionBase(converter_t converter)
        : m_converter(converter)
    {}

    bool operator()(const void *in, void *out)
    {
        return m_converter(*this, in, out);
    }
private:
    converter_t m_converter;
};

template<typename From, typename To, typename F>
struct ConvertFunctor: ConvertFunctionBase
{
    using this_t = ConvertFunctor<From, To, F>;

    explicit ConvertFunctor(F func)
        : ConvertFunctionBase{convert}, m_func{std::move(func)}
    {}

    static bool convert(const ConvertFunctionBase &self, const void *in, void *out)
    {
        auto _this = static_cast<this_t>(self);
        auto from = static_cast<const From*>(in);
        auto to = static_cast<To*>(out);
        *to = _this->m_func(*from);
        return true;
    }

private:
    F m_func;
};

template<typename From, typename To, typename F>
void MetaType::registerConverter(F func)
{
    static ConvertFunctor<From, To, F> converter{std::move(func)};
}

template<typename From, typename To>
void MetaType::registerConverter()
{
    registerConverter<From, To>(default_convert<From, To>);
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
    F(const void*, 38) \
    F(const char*, 39) \
    F(const wchar_t*, 40) \

#define DEFINE_STATIC_METATYPE_ID(NAME, TYPEID) \
template<> inline constexpr MetaType_ID metaTypeId<NAME>() \
{ return MetaType_ID{TYPEID}; }

FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_METATYPE_ID)

#undef DEFINE_STATIC_METATYPE_ID

} //namespace rtti

DLL_PUBLIC std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value);

#endif // METATYPE_H
