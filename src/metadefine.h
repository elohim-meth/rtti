#ifndef METADEFINE_H
#define METADEFINE_H

#include "metaconstructor.h"
#include "metamethod.h"
#include "metaproperty.h"
#include "metaenum.h"
#include "metaclass.h"
#include "metanamespace.h"
#include "metacontainer.h"
#include "metaitem.h"
#include "signature.h"

#include <function_traits.h>

#include <stack>
#include <cassert>
#include <type_traits>

namespace rtti {

//forward
template<typename, typename> class meta_define;

namespace internal {

using container_stack_t = std::stack<MetaContainer*>;

template<typename DerivedType, typename BaseType>
static const void* metacast_to_base(const void *value)
{
    return
        static_cast<const void*>(
            static_cast<const BaseType*>(
                static_cast<const DerivedType*>(value)));
}

template<typename T, typename F>
struct DefinitionCallbackHolder: IDefinitionCallbackHolder
{
    DefinitionCallbackHolder(const F &func)
        : m_func(func)
    {}

    DefinitionCallbackHolder(F &&func)
        : m_func(std::move(func))
    {}

    void invoke(MetaContainer &container)
    {
        container_stack_t stack;
        m_func(meta_define<T, void>{&container, &container, &stack});
    }

private:
    F m_func;
};

struct void_static_func{};
struct return_static_func{};
struct void_member_func {};
struct return_member_func {};

template<typename F>
using method_invoker_tag =
conditional_t<std::is_void<typename mpl::function_traits<F>::result_type>::value,
    conditional_t<std::is_void<typename mpl::function_traits<F>::class_type>::value, void_static_func, void_member_func>,
    conditional_t<std::is_void<typename mpl::function_traits<F>::class_type>::value, return_static_func, return_member_func>>;

template<typename F, typename Tag> struct method_invoker;

using argument_array_t = std::array<argument const*, IMethodInvoker::MaxNumberOfArguments>;

inline argument_array_t pack_arguments(std::size_t count,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
{
    argument_array_t result = {{
        &arg0, &arg1, &arg2, &arg3, &arg4,
        &arg5, &arg6, &arg7, &arg8, &arg9}};

    for (const auto &item: result)
    {
        if (item->empty())
            break;
        --count;
    }

    if (count != 0)
        throw invoke_error{"Invalid number of arguments"};

    return result;
}

template<typename F>
struct method_invoker<F, void_static_func>
{
    using Args = typename mpl::function_traits<F>::args;

    static_assert(mpl::typelist_size<Args>::value <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return true; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<void>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(char const *name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, args, argument_indexes_t{});
    }

    static variant invoke(F, variant const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&)
    { assert(false); return variant::empty_variant; }
private:
    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<Args, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, argument_array_t const &args, mpl::index_sequence<I...>)
    {
        func(args[I]->value<argument_get_t<I>>()...);
        return variant::empty_variant;
    }
};

template<typename F>
struct method_invoker<F, return_static_func>
{
    using Args = typename mpl::function_traits<F>::args;
    using Result = typename mpl::function_traits<F>::result_type;

    static_assert(mpl::typelist_size<Args>::value <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return true; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<Result>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(char const *name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, args, argument_indexes_t{}, result_is_reference{});
    }

    static variant invoke(F, const variant&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&)
    { assert(false); return variant::empty_variant; }
private:
    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<Args, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args>;
    using result_is_reference = is_reference_t<Result>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, argument_array_t const &args,
                              mpl::index_sequence<I...>, std::false_type)
    {
        return func(args[I]->value<argument_get_t<I>>()...);
    }
    template<std::size_t ...I>
    static variant invoke_imp(F func, argument_array_t const &args,
                              mpl::index_sequence<I...>, std::true_type)
    {
        return std::ref(func(args[I]->value<argument_get_t<I>>()...));
    }
};

template<typename F>
struct method_invoker<F, void_member_func>
{
    using Args = typename mpl::function_traits<F>::args;

    static_assert(mpl::typelist_size<Args>::value <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return false; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<void>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(const char *name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          const variant &instance,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F func,
                          variant &instance,
                          const argument &arg0, const argument &arg1,
                          const argument &arg2, const argument &arg3,
                          const argument &arg4, const argument &arg5,
                          const argument &arg6, const argument &arg7,
                          const argument &arg8, const argument &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&,
                          argument const&, argument const&)
    { assert(false); return variant::empty_variant; }
private:
    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<Args, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args>;
    //
    using C = typename mpl::function_traits<F>::class_type;
    using is_const = typename mpl::function_traits<F>::is_const;
    using class_t = conditional_t<is_const::value, const C, C>;
    using class_ref_t = add_lvalue_reference_t<class_t>;
    using class_ptr_t = add_pointer_t<class_t>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    static class_ref_t class_selector(variant const&, std::false_type)
    {
        throw bad_variant_cast{std::string{"Incompatible types: "} +
                               "const rtti::variant& -> " + mpl::type_name<class_ref_t>()};
    }

    static class_ref_t class_selector(variant const &instance, std::true_type)
    {
        return instance.value<class_t>();
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant const &instance,
                              argument_array_t const &args, mpl::index_sequence<I...>)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            (class_selector(instance, is_const{}).*func)(args[I]->value<argument_get_t<I>>()...);
        else if (type.isClassPtr())
            (instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...);
        return variant::empty_variant;
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant &instance,
                              argument_array_t const &args, mpl::index_sequence<I...>)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            (instance.value<class_t>().*func)(args[I]->value<argument_get_t<I>>()...);
        else if (type.isClassPtr())
            (instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...);
        return variant::empty_variant;
    }
};

template<typename F>
struct method_invoker<F, return_member_func>
{
    using Args = typename mpl::function_traits<F>::args;
    using Result = typename mpl::function_traits<F>::result_type;

    static_assert(mpl::typelist_size<Args>::value <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return false; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<Result>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(const char *name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          const variant &instance,
                          const argument &arg0, const argument &arg1,
                          const argument &arg2, const argument &arg3,
                          const argument &arg4, const argument &arg5,
                          const argument &arg6, const argument &arg7,
                          const argument &arg8, const argument &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F func,
                          variant &instance,
                          const argument &arg0, const argument &arg1,
                          const argument &arg2, const argument &arg3,
                          const argument &arg4, const argument &arg5,
                          const argument &arg6, const argument &arg7,
                          const argument &arg8, const argument &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size<Args>::value,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F,
                          const argument&, const argument&,
                          const argument&, const argument&,
                          const argument&, const argument&,
                          const argument&, const argument&,
                          const argument&, const argument&)
    { assert(false); return variant::empty_variant; }
private:
    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<Args, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args>;
    using result_is_reference = is_reference_t<Result>;
    //
    using C = typename mpl::function_traits<F>::class_type;
    using is_const = typename mpl::function_traits<F>::is_const;
    using class_t = conditional_t<is_const::value, C const, C>;
    using class_ref_t = add_lvalue_reference_t<class_t>;
    using class_ptr_t = add_pointer_t<class_t>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<typename R>
    static variant reference_selector(R &&result, std::false_type)
    {
        return std::forward<R>(result);
    }

    template<typename R>
    static variant reference_selector(R &&result, std::true_type)
    {
        return std::ref(std::forward<R>(result));
    }

    static class_ref_t class_selector(variant const&, std::false_type)
    {
        throw bad_variant_cast{std::string{"Incompatible types: "} +
                               "const rtti::variant& -> " + mpl::type_name<class_ref_t>()};
    }

    static class_ref_t class_selector(variant const &instance, std::true_type)
    {
        return instance.value<class_t>();
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant const &instance, argument_array_t const &args,
                              mpl::index_sequence<I...>)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            return reference_selector((class_selector(instance, is_const{}).*func)(args[I]->value<argument_get_t<I>>()...),
                                      result_is_reference{});
        else if (type.isClassPtr())
            return reference_selector((instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...),
                                      result_is_reference{});
        return variant::empty_variant;
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant &instance, argument_array_t const &args,
                              mpl::index_sequence<I...>)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            return reference_selector((instance.value<class_t>().*func)(args[I]->value<argument_get_t<I>>()...),
                                      result_is_reference{});
        else if (type.isClassPtr())
            return reference_selector((instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...),
                                      result_is_reference{});
        return variant::empty_variant;
    }
};

template<typename F>
struct MethodInvoker: IMethodInvoker
{
    using invoker_t = method_invoker<F, method_invoker_tag<F>>;

    MethodInvoker(const F &func) noexcept
        : m_func(func)
    {}
    MethodInvoker(F &&func) noexcept
        : m_func(std::move(func))
    {}

    bool isStatic() const override
    { return invoker_t::isStatic(); }

    MetaType_ID returnTypeId() const override
    { return invoker_t::returnTypeId(); }

    std::vector<MetaType_ID> parametersTypeId() const override
    { return invoker_t::parametersTypeId(); }

    std::string signature(const char *name) const override
    { return invoker_t::signature(name); }

    variant invoke_static(argument arg0 = argument{}, argument arg1 = argument{},
                          argument arg2 = argument{}, argument arg3 = argument{},
                          argument arg4 = argument{}, argument arg5 = argument{},
                          argument arg6 = argument{}, argument arg7 = argument{},
                          argument arg8 = argument{}, argument arg9 = argument{}) const override
    {
        return invoker_t::invoke(m_func,
                                         arg0, arg1, arg2, arg3, arg4,
                                         arg5, arg6, arg7, arg8, arg9);
    }

    variant invoke_method(variant const &instance,
                          argument arg0 = argument{}, argument arg1 = argument{},
                          argument arg2 = argument{}, argument arg3 = argument{},
                          argument arg4 = argument{}, argument arg5 = argument{},
                          argument arg6 = argument{}, argument arg7 = argument{},
                          argument arg8 = argument{}, argument arg9 = argument{}) const
    {
        return invoker_t::invoke(m_func, instance,
                                         arg0, arg1, arg2, arg3, arg4,
                                         arg5, arg6, arg7, arg8, arg9);
    }

    variant invoke_method(variant &instance,
                          argument arg0 = argument{}, argument arg1 = argument{},
                          argument arg2 = argument{}, argument arg3 = argument{},
                          argument arg4 = argument{}, argument arg5 = argument{},
                          argument arg6 = argument{}, argument arg7 = argument{},
                          argument arg8 = argument{}, argument arg9 = argument{}) const
    {
        return invoker_t::invoke(m_func, instance,
                                         arg0, arg1, arg2, arg3, arg4,
                                         arg5, arg6, arg7, arg8, arg9);
    }
private:
    const F m_func;
};

template<typename C, typename ...Args>
struct ConstructorInvoker: IConstructorInvoker
{
    static_assert(sizeof...(Args) <= MaxNumberOfArguments, "Maximum supported arguments: 10");
    static_assert((sizeof...(Args) > 0) || std::is_default_constructible<C>::value,
                  "Type is not default constructible");
    static_assert((sizeof...(Args) == 0) || std::is_constructible<C, Args...>::value,
                  "Type can not be constructed with given arguments");

    template<std::size_t I>
    using argument_get_t = mpl::typelist_get_t<mpl::type_list<Args...>, I>;
    using argument_indexes_t = mpl::index_sequence_for_t<Args...>;

    bool isStatic() const override
    {
        return true;
    }

    MetaType_ID returnTypeId() const override
    {
        return metaTypeId<C>();
    }

    std::vector<MetaType_ID> parametersTypeId() const override
    {
        return {metaTypeId<Args>()...};
    }

    std::string signature(char const*) const override
    {
        return signature_imp(argument_indexes_t{});
    }

    variant invoke_static(argument arg0 = argument{}, argument arg1 = argument{},
                          argument arg2 = argument{}, argument arg3 = argument{},
                          argument arg4 = argument{}, argument arg5 = argument{},
                          argument arg6 = argument{}, argument arg7 = argument{},
                          argument arg8 = argument{}, argument arg9 = argument{}) const override
    {
        auto const &args = pack_arguments(sizeof...(Args),
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(args, argument_indexes_t{});
    }

    variant invoke_method(variant const&,
                          argument, argument, argument, argument, argument,
                          argument, argument, argument, argument, argument) const override
    { assert(false); return variant::empty_variant; }

    variant invoke_method(variant&,
                          argument, argument, argument, argument, argument,
                          argument, argument, argument, argument, argument) const override
    { assert(false); return variant::empty_variant; }
private:
    static constexpr const char* signature_imp(mpl::index_sequence<>)
    {
        return "default constructor";
    }

    static std::string signature_imp(mpl::index_sequence<0>)
    {
        using Arg = argument_get_t<0>;
        if (std::is_same<decay_t<Arg>, C>::value)
        {
            if (std::is_rvalue_reference<Arg>::value)
                return "move constructor";
            else
                return "copy constructor";
        }
        return ::rtti::signature<Args...>::get("constructor");
    }

    template<std::size_t ...I>
    static std::string signature_imp(mpl::index_sequence<I...>)
    {
        return ::rtti::signature<Args...>::get("constructor");
    }

    template<std::size_t ...I>
    static variant invoke_imp(argument_array_t const &args, mpl::index_sequence<I...>)
    {
        return C(args[I]->value<argument_get_t<I>>()...);
    }
};

template<typename P> struct property_type;

template<typename P>
struct property_type<P*>: mpl::identity<P>
{};

template<typename C, typename T>
struct property_type<T (C::*)>: mpl::identity<T>
{};

template<typename P>
using property_type_t = typename property_type<P>::type;

template<typename P> struct property_class;

template<typename C, typename T>
struct property_class<T (C::*)>: mpl::identity<C>
{};

template<typename P>
using property_class_t = typename property_class<P>::type;

struct static_pointer{};
struct member_pointer{};

template<typename P>
using property_invoker_tag = conditional_t<std::is_member_pointer<P>::value, member_pointer, static_pointer>;

template<typename P, typename Tag> struct property_invoker;

template<typename P>
struct property_invoker<P, static_pointer>
{
    static_assert(std::is_pointer<P>::value && !std::is_function<P>::value,
                  "Type should be pointer");

    static bool isStatic()
    { return true; }

    static MetaType_ID typeId()
    { return metaTypeId<T>(); }

    static bool readOnly()
    { return IsReadOnly::value; }

    static variant get_static(P property)
    { return std::cref(*property); }

    static void set_static(P property, const argument &arg)
    {
        set_static_selector(property, arg, IsReadOnly{});
    }

    static variant get_field(P, const variant&)
    { assert(false); return variant::empty_variant; }

    static void set_field(P, variant const &, const argument&)
    { assert(false); }

    static void set_field(P, variant&, const argument&)
    { assert(false); }

private:
    using T = property_type_t<P>;
    using IsReadOnly = is_const_t<T>;

    static void set_static_selector(P, const argument&, std::true_type)
    {
        throw invoke_error{"Write to readonly property"};
    }

    static void set_static_selector(P property, const argument &arg, std::false_type)
    {
        *property = arg.value<T>();
    }
};

template<typename P>
struct property_invoker<P, member_pointer>
{
    static_assert(std::is_member_object_pointer<P>::value,
                  "Type should be member object pointer");

    static bool isStatic()
    { return false; }

    static MetaType_ID typeId()
    { return metaTypeId<T>(); }

    static bool readOnly()
    { return IsReadOnly::value; }

    static variant get_static(P)
    { assert(false); return variant::empty_variant; }

    static void set_static(P, const argument&)
    { assert(false); }

    static variant get_field(P property, const variant &instance)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            return std::ref(instance.value<const C>().*property);
        else if (type.isClassPtr())
            return std::ref(instance.to<const C*>()->*property);
        return variant::empty_variant;
    }

    static void set_field(P property, variant const &instance, argument const &arg)
    {
        set_field_selector(property, instance, arg, IsReadOnly{});
    }

    static void set_field(P property, variant &instance, argument const &arg)
    {
        set_field_selector(property, instance, arg, IsReadOnly{});
    }
private:
    using C = property_class_t<P>;
    using T = property_type_t<P>;
    using IsReadOnly = is_const_t<T>;
    using class_ref_t = add_lvalue_reference_t<C>;

    static void set_field_selector(P, variant&, argument const&, std::true_type)
    {
        throw invoke_error{"Write to readonly property"};
    }

    static void set_field_selector(P, variant const&, argument const&, std::true_type)
    {
        throw invoke_error{"Write to readonly property"};
    }

    static void set_field_selector(P property, variant const &instance, argument const &arg, std::false_type)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
        {
            throw bad_variant_cast{std::string{"Incompatible types: "} +
                                   "const rtti::variant& -> " + mpl::type_name<class_ref_t>()};
        }
        else if (type.isClassPtr())
        {
            instance.to<C*>()->*property = arg.value<T>();
        }
    }

    static void set_field_selector(P property, variant &instance, argument const &arg, std::false_type)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            instance.value<C>().*property = arg.value<T>();
        else if (type.isClassPtr())
            instance.to<C*>()->*property = arg.value<T>();
    }
};

template<typename P>
struct PropertyInvoker: IPropertyInvoker
{
    using invoker_t = property_invoker<P, property_invoker_tag<P>>;

    PropertyInvoker(P prop) noexcept
        : m_prop(prop)
    {}

    bool isStatic() const override
    { return invoker_t::isStatic(); }

    MetaType_ID typeId() const override
    { return invoker_t::typeId(); }

    bool readOnly() const override
    { return invoker_t::readOnly(); }

    variant get_static() const override
    { return invoker_t::get_static(m_prop); }

    void set_static(argument arg) const override
    { invoker_t::set_static(m_prop, arg); }

    variant get_field(const variant &instance) const override
    { return invoker_t::get_field(m_prop, instance); }

    void set_field(variant const &instance, argument arg) const override
    { invoker_t::set_field(m_prop, instance, arg); }

    void set_field(variant &instance, argument arg) const override
    { invoker_t::set_field(m_prop, instance, arg); }

private:
    P m_prop;
};

template<typename G, typename S>
struct PropertyInvokerEx: IPropertyInvoker
{
    PropertyInvokerEx(G get, S set) noexcept
        : m_get(get), m_set(set)
    {}

    bool isStatic() const override
    { return std::is_function<G>::value; }

    MetaType_ID typeId() const override
    { return metaTypeId<T>(); }

    bool readOnly() const override
    { return false; }

    variant get_static() const override
    { return MethodInvoker<G>(m_get).invoke_static(); }

    void set_static(argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_static(std::move(arg)); }

    variant get_field(const variant &instance) const override
    { return MethodInvoker<G>{m_get}.invoke_method(instance); }

    void set_field(variant const &instance, argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_method(instance, std::move(arg)); }

    void set_field(variant &instance, argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_method(instance, std::move(arg)); }

private:
    static constexpr bool valid = conditional_t
    <(std::is_function<G>::value && (std::is_void<S>::value || std::is_function<S>::value)) ||
     (std::is_member_function_pointer<G>::value && (std::is_void<S>::value || std::is_member_function_pointer<S>::value))
    ,std::true_type, std::false_type>::value;
    static_assert(valid, "Get and Set methods should be simultaneously static method or pointer to member");

    using GTraits = mpl::function_traits<G>;
    using STraits = mpl::function_traits<S>;

    using T = typename GTraits::result_type;
    static_assert(!std::is_void<T>::value,
                  "Get method should have non void result type");
    static_assert(GTraits::arity::value == 0,
                  "Get method shouldn't have any parameters");
    static_assert(std::is_void<typename STraits::result_type>::value,
                  "Set method should have void result type");
    static_assert(STraits::arity::value == 1,
                  "Set method should have one parameters");
    using Arg = typename STraits::template arg<0>::type;
    static_assert(std::is_same<decay_t<T>, decay_t<Arg>>::value,
                  "Get method return type and Set method parameter type do not match");
    G m_get;
    S m_set;
};

} // namespace internal

template<typename T, typename MB = void>
class meta_define
{
public:
    using this_t = meta_define<T, MB>;

    template<typename V>
    this_t _attribute(const char *name, V &&value)
    {
        assert(m_currentItem);
        if (m_currentItem)
            m_currentItem->setAttribute(name, std::forward<V>(value));
        return std::move(*this);
    }

    template<typename V>
    this_t _attribute(const std::string &name, V &&value)
    {
        return _attribute(name.c_str(), std::forward<V>(value));
    }

    meta_define<void, this_t> _namespace(const char *name)
    {
        static_assert(std::is_void<T>::value, "Namespace can be defined only in another namespace");
        assert(m_currentContainer && m_currentContainer->category() == mcatNamespace && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaNamespace::create(name, *m_currentContainer);
        m_currentItem = m_currentContainer;
        return meta_define<void, this_t>{m_currentItem, m_currentContainer, m_containerStack};
    }

    meta_define<void, this_t> _namespace(const std::string &name)
    {
        return _namespace(name.c_str());
    }

    template<typename C>
    meta_define<C, this_t> _class(const char *name)
    {
        static_assert(std::is_class<C>::value, "Template argument <C> must be class");
        static_assert(!std::is_same<T, C>::value, "Recursive class definition");
        static_assert(std::is_void<T>::value || std::is_class<T>::value,
                      "Class can be defined in namespace or anther class");
        assert(m_currentContainer && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaClass::create(name, *m_currentContainer, metaTypeId<C>());
        m_currentItem = m_currentContainer;

        meta_define<C, this_t> result {m_currentItem, m_currentContainer, m_containerStack};
        result.default_constructor_selector(std::is_default_constructible<C>{});
        result.copy_constructor_selector(std::is_copy_constructible<C>{});
        result.move_constructor_selector(std::is_move_constructible<C>{});
        return std::move(result);
    }

    template<typename C>
    meta_define<C, this_t> _class(const std::string &name)
    {
        return _class<C>(name.c_str());
    }

    template<typename C>
    meta_define<C, this_t> _class()
    {
        return _class<C>(mpl::type_name<C>());
    }

    template<typename F>
    this_t _lazy(F &&func)
    {
        static_assert(std::is_void<T>::value || std::is_class<T>::value,
                      "Deferred definition supported only for namespaces and class types");

        assert(m_currentContainer);
        using holder_t = internal::DefinitionCallbackHolder<T, decay_t<F>>;
        m_currentContainer->setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder>{
                                                  new holder_t{std::forward<F>(func)}});
        return std::move(*this);
    }

    template<typename ...B>
    this_t _base()
    {
        static_assert(std::is_class<T>::value, "Base class can be defined only for class types");
        assert(m_currentContainer && m_currentContainer->category() == mcatClass);

        addBaseTypeList<mpl::type_list<B...>>(
                    static_cast<MetaClass*>(m_currentContainer),
                    mpl::index_sequence_for_t<B...>{});
        return std::move(*this);
    }

    template<typename E>
    this_t _enum(const char *name)
    {
        assert(m_currentContainer);
        m_currentItem = MetaEnum::create(name, *m_currentContainer, metaTypeId<E>());
        return std::move(*this);
    }

    template<typename E>
    this_t _enum(const std::string &name)
    {
        return _enum<E>(name.c_str());
    }

    template<typename V>
    this_t _element(const char *name, V &&value)
    {
        assert(m_currentItem);
        auto category = m_currentItem->category();
        assert(category == mcatEnum);
        if (category == mcatEnum)
        {
            auto e = static_cast<MetaEnum*>(m_currentItem);
            e->addElement(name, std::forward<V>(value));
        }
        return std::move(*this);
    }

    template<typename V>
    this_t _element(const std::string &name, V &&value)
    {
        return _element(name.c_str(), std::forward<V>(value));
    }

    template<typename ...Args>
    this_t _constructor(const char *name = nullptr)
    {
        static_assert(std::is_class<T>::value, "Constructor can be defined only for class types");
        assert(m_currentContainer && m_currentContainer->category() == mcatClass);
        MetaConstructor::create(name, *m_currentContainer,
                                std::unique_ptr<IConstructorInvoker>{
                                    new internal::ConstructorInvoker<T, Args...>{}});
        register_converting_constructor<mpl::type_list<Args...>>(is_converting_constructor_t<T, Args...>{});
        return std::move(*this);
    }

    template<typename ...Args>
    this_t _constructor(const std::string &name)
    {
        return _constructor<Args...>(name.c_str());
    }

    template<typename F>
    this_t _method(const char *name, F &&func)
    {
        static_assert(std::is_void<T>::value || std::is_class<T>::value,
                      "Method can be defined in namespace or class");
        assert(m_currentContainer);
        MetaMethod::create(name, *m_currentContainer,
                           std::unique_ptr<IMethodInvoker>{
                                new internal::MethodInvoker<decay_t<F>>{std::forward<F>(func)}});
        return std::move(*this);
    }

    template<typename F>
    this_t _method(const std::string &name, F &&func)
    {
        return _method(name.c_str(), std::forward<F>(func));
    }

    template<typename P>
    this_t _property(const char *name, P &&prop)
    {
        static_assert(std::is_void<T>::value || std::is_class<T>::value,
                      "Propery can be defined in namespace or class");
        assert(m_currentContainer);
        MetaProperty::create(name, *m_currentContainer,
                           std::unique_ptr<IPropertyInvoker>{
                                new internal::PropertyInvoker<decay_t<P>>{std::forward<P>(prop)}});
        return std::move(*this);
    }

    template<typename G, typename S>
    this_t _property(const char *name, G &&get, S &&set)
    {
        static_assert(std::is_void<T>::value || std::is_class<T>::value,
                      "Propery can be defined in namespace or class");
        assert(m_currentContainer);
        MetaProperty::create(name, *m_currentContainer,
                           std::unique_ptr<IPropertyInvoker>{
                                new internal::PropertyInvokerEx<decay_t<G>, decay_t<S>>{std::forward<G>(get), std::forward<S>(set)}});
        return std::move(*this);
    }

    MB _end()
    {
        static_assert(!std::is_void<MB>::value, "Container stack is EMPTY");
        assert(m_containerStack && !m_containerStack->empty());

        m_currentContainer = m_containerStack->top();
        m_currentItem = m_currentContainer;
        m_containerStack->pop();
        return MB{m_currentItem, m_currentContainer, m_containerStack};
    }

protected:
    meta_define() = default;
    meta_define(const meta_define&) = delete;
    meta_define& operator=(const meta_define&) = delete;
    meta_define(meta_define&&) = default;
    meta_define& operator=(meta_define&&) = default;

    meta_define(MetaItem *currentItem, MetaContainer *currentContainer,
                internal::container_stack_t *containerStack)
        : m_currentItem{currentItem},
          m_currentContainer{currentContainer},
          m_containerStack{containerStack}
    {}

    MetaItem *m_currentItem = nullptr;
    MetaContainer *m_currentContainer = nullptr;
    internal::container_stack_t *m_containerStack = nullptr;

private:
    template<typename C>
    struct check_is_class
    {
        static_assert(std::is_class<C>::value, "Type <C> is not a class");
        static_assert(std::is_same<C, full_decay_t<C>>::value, "Type <C> is not a class");
        using type = C;
    };

    template<typename L, std::size_t ...I>
    static void addBaseTypeList(MetaClass *item, mpl::index_sequence<I...>)
    {
        // check that every type is class
        typename mpl::typelist_map<L, check_is_class>::type tmp;
        (void) tmp;

        EXPAND(
            item->addBaseClass(metaTypeId<mpl::typelist_get_t<L, I>>(),
                               &internal::metacast_to_base<T, mpl::typelist_get_t<L, I>>)
        );
    }

    void default_constructor_selector(std::false_type) {}
    void default_constructor_selector(std::true_type)
    {
        _constructor();
    }

    void copy_constructor_selector(std::false_type) {}
    void copy_constructor_selector(std::true_type)
    {
        _constructor<const T&>();
    }

    void move_constructor_selector(std::false_type) {}
    void move_constructor_selector(std::true_type)
    {
        _constructor<T&&>();
    }

    template<typename L>
    void register_converting_constructor(std::false_type) {}
    template<typename L>
    void register_converting_constructor(std::true_type)
    {
        using Arg = mpl::typelist_get_t<L, 0>;
        MetaType::registerConverter(&internal::default_convert<Arg, T>);
    }

    template<typename, typename> friend struct internal::DefinitionCallbackHolder;
    template<typename, typename> friend class rtti::meta_define;
};

class DLL_PUBLIC meta_global: public meta_define<void>
{
protected:
    meta_global(MetaItem *currentItem, MetaContainer *currentContainer)
        : meta_define<void>{currentItem, currentContainer, nullptr}
    {
        m_containerStack = &m_container;
    }
private:
    internal::container_stack_t m_container;

    friend DLL_PUBLIC meta_global global_define();
};

DLL_PUBLIC meta_global global_define();

} // namespace rtti

#endif // METADEFINE_H
