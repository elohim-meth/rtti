#ifndef METADEFINE_H
#define METADEFINE_H

#include <rtti/metaconstructor.h>
#include <rtti/metaenum.h>
#include <rtti/metaclass.h>
#include <rtti/metanamespace.h>
#include <rtti/metacontainer.h>
#include <rtti/metaitem.h>
#include <rtti/signature.h>

#include <stack>
#include <cassert>
#include <type_traits>

namespace rtti {

//forward
template<typename, typename> class meta_define;

namespace internal {

using container_stack_t = std::stack<MetaContainer*>;

template<typename DerivedType, typename BaseType>
static void const * metacast_to_base(void const *value)
{
    return
        static_cast<void const *>(
            static_cast<BaseType const *>(
                static_cast<DerivedType const *>(value)));
}

template<typename T, typename F>
struct DefinitionCallbackHolder: IDefinitionCallbackHolder
{
    DefinitionCallbackHolder(F const &func)
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
    using meta_define_t = meta_define<T, void>;
    static_assert (std::is_invocable_v<F, meta_define_t>,
                   "Invalid lazy definition signature!");
    F m_func;
};

template<typename T, typename F>
inline auto make_definition_callback(F &&func)
{
    using callback_holder_t = DefinitionCallbackHolder<T, std::decay_t<F>>;
    return std::unique_ptr<IDefinitionCallbackHolder>{new callback_holder_t{std::forward<F>(func)}};
}

struct void_static_func{};
struct return_static_func{};
struct void_member_func {};
struct return_member_func {};

template<typename F>
using method_invoker_tag =
std::conditional_t<std::is_void_v<typename mpl::function_traits<F>::result_type>,
    std::conditional_t<std::is_void_v<typename mpl::function_traits<F>::class_type>, void_static_func, void_member_func>,
    std::conditional_t<std::is_void_v<typename mpl::function_traits<F>::class_type>, return_static_func, return_member_func>>;

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

    for (auto const &item: result)
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

    static_assert(mpl::typelist_size_v<Args> <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return true; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<void>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(std::string_view name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
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

    static_assert(mpl::typelist_size_v<Args> <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return true; }

    static MetaType_ID returnTypeId()
    { return metaTypeId<Result>(); }

    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }

    static std::string signature(std::string_view name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke(func, args, argument_indexes_t{});
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

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<std::size_t ...I>
    static variant invoke(F func, argument_array_t const &args, mpl::index_sequence<I...>)
    {
        if constexpr(std::is_reference_v<Result>)
            return std::ref(func(args[I]->value<argument_get_t<I>>()...));
        else
            return func(args[I]->value<argument_get_t<I>>()...);
    }
};

template<typename F>
struct method_invoker<F, void_member_func>
{
    using Args = typename mpl::function_traits<F>::args;

    static_assert(mpl::typelist_size_v<Args> <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return false; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<void>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(std::string_view name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          variant const &instance,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F func,
                          variant &instance,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
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
    using is_const_method = typename mpl::function_traits<F>::is_const;
    using class_t = std::conditional_t<is_const_method::value, std::add_const_t<C>, C>;
    using class_ref_t = std::add_lvalue_reference_t<class_t>;
    using class_ptr_t = std::add_pointer_t<class_t>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant const &instance,
                              argument_array_t const &args, mpl::index_sequence<I...>)
    {
        using namespace std::literals;
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
        {
            if constexpr(is_const_method::value)
                (instance.ref<class_t>().*func)(args[I]->value<argument_get_t<I>>()...);
            else
                throw bad_variant_cast{"Incompatible types: const rtti::variant& -> "s +
                                       type_name<class_ref_t>()};
        }
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
            (instance.ref<class_t>().*func)(args[I]->value<argument_get_t<I>>()...);
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

    static_assert(mpl::typelist_size_v<Args> <= IMethodInvoker::MaxNumberOfArguments,
                  "Maximum supported arguments: 10");

    static bool isStatic()
    { return false; }
    static MetaType_ID returnTypeId()
    { return metaTypeId<Result>(); }
    static std::vector<MetaType_ID> parametersTypeId()
    { return parametersTypeId(argument_indexes_t{}); }
    static std::string signature(std::string_view name)
    { return f_signature<F>::get(name); }

    static variant invoke(F func,
                          variant const &instance,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
                                          arg0, arg1, arg2, arg3, arg4,
                                          arg5, arg6, arg7, arg8, arg9);
        return invoke_imp(func, instance, args, argument_indexes_t{});
    }

    static variant invoke(F func,
                          variant &instance,
                          argument const &arg0, argument const &arg1,
                          argument const &arg2, argument const &arg3,
                          argument const &arg4, argument const &arg5,
                          argument const &arg6, argument const &arg7,
                          argument const &arg8, argument const &arg9)
    {
        auto const &args = pack_arguments(mpl::typelist_size_v<Args>,
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
    using is_const_method = typename mpl::function_traits<F>::is_const;
    using class_t = std::conditional_t<is_const_method::value, std::add_const_t<C>, C>;
    using class_ref_t = std::add_lvalue_reference_t<class_t>;
    using class_ptr_t = std::add_pointer_t<class_t>;

    template<std::size_t ...I>
    static std::vector<MetaType_ID> parametersTypeId(mpl::index_sequence<I...>)
    {
        return { metaTypeId<argument_get_t<I>>()... };
    }

    template<typename R>
    static variant reference_get(R &&result)
    {
        if constexpr(std::is_reference_v<Result>)
            return std::ref(std::forward<R>(result));
        else
            return std::forward<R>(result);
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant const &instance, argument_array_t const &args,
                              mpl::index_sequence<I...>)
    {
        using namespace std::literals;
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
        {
            if constexpr(is_const_method::value)
                return reference_get((instance.ref<class_t>().*func)(args[I]->value<argument_get_t<I>>()...));
            else
                throw bad_variant_cast{"Incompatible types: const rtti::variant& -> "s +
                                       type_name<class_ref_t>()};
        }
        else if (type.isClassPtr())
            return reference_get((instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...));

        return variant::empty_variant;
    }

    template<std::size_t ...I>
    static variant invoke_imp(F func, variant &instance, argument_array_t const &args,
                              mpl::index_sequence<I...>)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            return reference_get((instance.ref<class_t>().*func)(args[I]->value<argument_get_t<I>>()...));
        else if (type.isClassPtr())
            return reference_get((instance.to<class_ptr_t>()->*func)(args[I]->value<argument_get_t<I>>()...));

        return variant::empty_variant;
    }
};

template<typename F>
struct MethodInvoker: IMethodInvoker
{
    using invoker_t = method_invoker<F, method_invoker_tag<F>>;

    MethodInvoker(F const &func) noexcept
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

    std::string signature(std::string_view name) const override
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
                          argument arg8 = argument{}, argument arg9 = argument{}) const override
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
                          argument arg8 = argument{}, argument arg9 = argument{}) const override
    {
        return invoker_t::invoke(m_func, instance,
                                         arg0, arg1, arg2, arg3, arg4,
                                         arg5, arg6, arg7, arg8, arg9);
    }
private:
    F const m_func;
};

template <typename F>
inline auto make_method_invoker(F &&func)
{
    return std::unique_ptr<IMethodInvoker>{new MethodInvoker<std::decay_t<F>>{std::forward<F>(func)}};
}

template<typename C, typename ...Args>
struct ConstructorInvoker: IConstructorInvoker
{
    static_assert(sizeof...(Args) <= MaxNumberOfArguments, "Maximum supported arguments: 10");
    static_assert((sizeof...(Args) > 0) || std::is_default_constructible_v<C>,
                  "Type is not default constructible");
    static_assert((sizeof...(Args) == 0) || std::is_constructible_v<C, Args...>,
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

    std::string signature(std::string_view name) const override
    {
        using  args_size_t = std::integral_constant<int, sizeof... (Args)>;
        return signature(name, args_size_t{});
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
        return invoke(args, argument_indexes_t{});
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
    static constexpr char const* signature(std::string_view,
                                           std::integral_constant<int, 0>)
    {
        return DEFAULT_CONSTRUCTOR_SIG;
    }

    static std::string signature(std::string_view name,
                                 std::integral_constant<int, 1>)
    {
        using Arg = argument_get_t<0>;
        if constexpr(std::is_same_v<std::decay_t<Arg>, C>)
        {
            if constexpr(std::is_rvalue_reference_v<Arg>)
                return MOVE_CONSTRUCTOR_SIG;
            else
                return COPY_CONSTRUCTOR_SIG;
        }
        else
            return rtti::signature<Args...>::get(name);
    }

    template<int N>
    static std::string signature(std::string_view name,
                                 std::integral_constant<int, N>)
    {
        return ::rtti::signature<Args...>::get(name);
    }

    template<std::size_t ...I>
    static variant invoke(argument_array_t const &args,
                          mpl::index_sequence<I...>)
    {
        return C(args[I]->value<argument_get_t<I>>()...);
    }
};

template <typename C, typename ...Args>
inline auto make_constructor_invoker()
{
    return std::unique_ptr<IConstructorInvoker>{new ConstructorInvoker<C, Args...>{}};
}

template<typename P> struct property_type;

template<typename P>
struct property_type<P*>: mpl::identity<P>
{};

template<typename C, typename T>
struct property_type<T C::*>: mpl::identity<T>
{};

template<typename P>
using property_type_t = typename property_type<P>::type;

template<typename P> struct property_class;

template<typename C, typename T>
struct property_class<T C::*>: mpl::identity<C>
{};

template<typename P>
using property_class_t = typename property_class<P>::type;

struct static_pointer{};
struct member_pointer{};

template<typename P>
using property_invoker_tag = std::conditional_t<std::is_member_pointer_v<P>, member_pointer, static_pointer>;

template<typename P, typename Tag> struct property_invoker;

template<typename P>
struct property_invoker<P, static_pointer>
{
    static_assert(std::is_pointer_v<P> && !std::is_function_v<P>,
                  "Type should be pointer");

    static bool isStatic()
    { return true; }

    static MetaType_ID typeId()
    { return metaTypeId<T>(); }

    static bool readOnly()
    { return std::is_const_v<T>; }

    static variant get_static(P property)
    { return std::cref(*property); }

    static void set_static([[maybe_unused]] P property, argument const &arg)
    {
        if constexpr(std::is_const_v<T>)
            throw invoke_error{"Write to readonly property"};
        else
            *property = arg.value<T>();
    }

    static variant get_field(P, variant const&)
    { assert(false); return variant::empty_variant; }

    static void set_field(P, variant const&, argument const&)
    { assert(false); }

    static void set_field(P, variant&, argument const&)
    { assert(false); }

private:
    using T = property_type_t<P>;
};

template<typename P>
struct property_invoker<P, member_pointer>
{
    static_assert(std::is_member_object_pointer_v<P>,
                  "Type should be member object pointer");

    static bool isStatic()
    { return false; }

    static MetaType_ID typeId()
    { return metaTypeId<T>(); }

    static bool readOnly()
    { return std::is_const_v<T>; }

    static variant get_static(P)
    { assert(false); return variant::empty_variant; }

    static void set_static(P, argument const&)
    { assert(false); }

    static variant get_field(P property, variant const &instance)
    {
        auto type = MetaType{instance.typeId()};
        if (type.isClass())
            return std::ref(instance.ref<C const>().*property);
        else if (type.isClassPtr())
            return std::ref(instance.to<C const *>()->*property);
        return variant::empty_variant;
    }

    static void set_field(P property, variant const &instance, argument const &arg)
    {
        using namespace std::literals;
        if constexpr(std::is_const_v<T>)
            throw invoke_error{"Write to readonly property"};
        else
        {
            auto type = MetaType{instance.typeId()};
            if (type.isClass())
                throw bad_variant_cast{"Incompatible types: const rtti::variant& -> "s +
                                       type_name<class_ref_t>()};
            else if (type.isClassPtr())
                instance.to<C*>()->*property = arg.value<T>();
        }
    }

    static void set_field(P property, variant &instance, argument const &arg)
    {
        if constexpr(std::is_const_v<T>)
            throw invoke_error{"Write to readonly property"};
        else
        {
            auto type = MetaType{instance.typeId()};
            if (type.isClass())
                instance.ref<C>().*property = arg.value<T>();
            else if (type.isClassPtr())
                instance.to<C*>()->*property = arg.value<T>();
        }
    }
private:
    using C = property_class_t<P>;
    using T = property_type_t<P>;
    using class_ref_t = std::add_lvalue_reference_t<C>;
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

    variant get_field(variant const &instance) const override
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
    { return std::is_function_v<G>; }

    MetaType_ID typeId() const override
    { return metaTypeId<T>(); }

    bool readOnly() const override
    { return false; }

    variant get_static() const override
    { return MethodInvoker<G>(m_get).invoke_static(); }

    void set_static(argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_static(std::move(arg)); }

    variant get_field(variant const &instance) const override
    { return MethodInvoker<G>{m_get}.invoke_method(instance); }

    void set_field(variant const &instance, argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_method(instance, std::move(arg)); }

    void set_field(variant &instance, argument arg) const override
    { MethodInvoker<S>{m_set}.invoke_method(instance, std::move(arg)); }

private:
    static constexpr auto valid = conditional_v<
     (std::is_function_v<G> && (std::is_void_v<S> || std::is_function_v<S>))
     ||
     (std::is_member_function_pointer_v<G> && (std::is_void_v<S> || std::is_member_function_pointer_v<S>)),
     std::true_type, std::false_type>;
    static_assert(valid, "Get and Set methods should be simultaneously static method or pointer to member");

    using GTraits = mpl::function_traits<G>;
    using STraits = mpl::function_traits<S>;

    using T = typename GTraits::result_type;
    static_assert(!std::is_void_v<T>,
                  "Get method should have non void result type");
    static_assert(GTraits::arity::value == 0,
                  "Get method shouldn't have any parameters");
    static_assert(std::is_void_v<typename STraits::result_type>,
                  "Set method should have void result type");
    static_assert(STraits::arity::value == 1,
                  "Set method should have one parameters");
    using Arg = typename STraits::template arg<0>::type;
    static_assert(std::is_same_v<std::decay_t<T>, std::decay_t<Arg>>,
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
    this_t _attribute(std::string_view name, V &&value)
    {
        assert(m_currentItem);
        if (m_currentItem)
            m_currentItem->setAttribute(name, std::forward<V>(value), {});
        return *this;
    }

    meta_define<void, this_t> _namespace(std::string_view name)
    {
        static_assert(std::is_void_v<T>, "Namespace can be defined only in another namespace");
        assert(m_currentContainer && m_currentContainer->category() == mcatNamespace && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaNamespace::create(name, *m_currentContainer, {});
        m_currentItem = m_currentContainer;
        return meta_define<void, this_t>{m_currentItem, m_currentContainer, m_containerStack};
    }

    template<typename C>
    meta_define<C, this_t> _class(std::string_view name)
    {
        static_assert(std::is_class_v<C>, "Template argument <C> must be class");
        static_assert(!std::is_same_v<T, C>, "Recursive class definition");
        static_assert(std::is_void_v<T> || std::is_class_v<T>,
                      "Class can be defined in namespace or anther class");
        assert(m_currentContainer && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaClass::create(
            name, *m_currentContainer, metaTypeId<C>(), {});
        m_currentItem = m_currentContainer;

        meta_define<C, this_t> result {m_currentItem, m_currentContainer, m_containerStack};
        result.define_default_constructor();
        result.define_copy_constructor();
        result.define_move_constructor();
        return result;
    }

    template<typename C>
    meta_define<C, this_t> _class()
    {
        return _class<C>(type_name<C>());
    }

    template<typename F>
    this_t _lazy(F &&func)
    {
        static_assert(std::is_void_v<T> || std::is_class_v<T>,
                      "Deferred definition supported only for namespaces and class types");

        assert(m_currentContainer);
        m_currentContainer->setDeferredDefine(internal::make_definition_callback<T>(std::forward<F>(func)), {});
        return *this;
    }

    template<typename ...B>
    this_t _base()
    {
        static_assert(std::is_class_v<T>, "Base class can be defined only for class types");
        assert(m_currentContainer && m_currentContainer->category() == mcatClass);

        addBaseTypeList<mpl::type_list<B...>>(
            static_cast<MetaClass*>(m_currentContainer),
            mpl::index_sequence_for_t<B...>{});
        return *this;
    }

    template<typename E>
    this_t _enum(std::string_view name)
    {
        assert(m_currentContainer);
        m_currentItem = MetaEnum::create(name, *m_currentContainer, metaTypeId<E>(), {});
        return *this;
    }

    template<typename V>
    this_t _element(std::string_view name, V &&value)
    {
        assert(m_currentItem);
        auto category = m_currentItem->category();
        assert(category == mcatEnum);
        if (category == mcatEnum)
        {
            auto e = static_cast<MetaEnum*>(m_currentItem);
            e->addElement(name, std::forward<V>(value), {});
        }
        return *this;
    }

    template<typename ...Args>
    this_t _constructor(std::string_view name = "")
    {
        static_assert(std::is_class_v<T>, "Constructor can be defined only for class types");
        assert(m_currentContainer && m_currentContainer->category() == mcatClass);
        MetaConstructor::create(name, *m_currentContainer, internal::make_constructor_invoker<T, Args...>(), {});
        register_converting_constructor<mpl::type_list<Args...>>(is_converting_constructor_t<T, Args...>{});
        return *this;
    }

    template<typename F>
    this_t _method(std::string_view name, F &&func)
    {
        static_assert(std::is_void_v<T> || std::is_class_v<T>,
                      "Method can be defined in namespace or class");
        assert(m_currentContainer);
        MetaMethod::create(name, *m_currentContainer, internal::make_method_invoker(std::forward<F>(func)), {});
        return *this;
    }

    template<typename P>
    this_t _property(std::string_view name, P &&prop)
    {
        static_assert(std::is_void_v<T> || std::is_class_v<T>,
                      "Propery can be defined in namespace or class");
        assert(m_currentContainer);
        MetaProperty::create(name, *m_currentContainer, std::unique_ptr<IPropertyInvoker>{
                                new internal::PropertyInvoker<std::decay_t<P>>{std::forward<P>(prop)}},
                            {});
        return *this;
    }

    template<typename G, typename S>
    this_t _property(std::string_view name, G &&get, S &&set)
    {
        static_assert(std::is_void_v<T> || std::is_class_v<T>,
                      "Propery can be defined in namespace or class");
        assert(m_currentContainer);
        MetaProperty::create(name, *m_currentContainer, std::unique_ptr<IPropertyInvoker>{
                                new internal::PropertyInvokerEx<std::decay_t<G>, std::decay_t<S>>{
                                     std::forward<G>(get), std::forward<S>(set)}},
                             {});
        return *this;
    }

    MB _end()
    {
        static_assert(!std::is_void_v<MB>, "Container stack is EMPTY");
        assert(m_containerStack && !m_containerStack->empty());

        m_currentContainer = m_containerStack->top();
        m_currentItem = m_currentContainer;
        m_containerStack->pop();
        return MB{m_currentItem, m_currentContainer, m_containerStack};
    }

protected:
    meta_define() = default;
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
        static_assert(std::is_class_v<C>, "Type <C> is not a class");
        static_assert(std::is_same_v<C, full_decay_t<C>>, "Type <C> is not a class");
        using type = C;
    };

    template<typename L, std::size_t ...I>
    static void addBaseTypeList(MetaClass *item, mpl::index_sequence<I...>)
    {
        // check that every type is class
        static_assert(mpl::is_typelist_v<mpl::typelist_map_t<L, check_is_class>>, "");

        EXPAND(
            item->addBaseClass(metaTypeId<mpl::typelist_get_t<L, I>>(),
                &internal::metacast_to_base<T, mpl::typelist_get_t<L, I>>, {})
        )
    }

    void define_default_constructor()
    {
        if constexpr(std::is_default_constructible_v<T>)
            _constructor();
    }

    void define_copy_constructor()
    {
        if constexpr(std::is_copy_constructible_v<T>)
            _constructor<T const &>();
    }

    void define_move_constructor()
    {
        if constexpr(std::is_move_constructible_v<T>)
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

class RTTI_API meta_global: public meta_define<void>
{
protected:
    meta_global(MetaItem *currentItem, MetaContainer *currentContainer)
        : meta_define<void>{currentItem, currentContainer, nullptr}
    {
        m_containerStack = &m_container;
    }
private:
    internal::container_stack_t m_container;

    friend RTTI_API meta_global global_define();
};

RTTI_API meta_global global_define();

} // namespace rtti

#define RTTI_REGISTER                                                               \
static void rtti_auto_register_function_();                                         \
namespace                                                                           \
{                                                                                   \
    struct rtti_auto_register_t                                                     \
    {                                                                               \
        rtti_auto_register_t()                                                      \
        {                                                                           \
            rtti_auto_register_function_();                                         \
        }                                                                           \
    };                                                                              \
}                                                                                   \
static rtti_auto_register_t const CONCAT(auto_register_,__LINE__);                  \
static void rtti_auto_register_function_()


#endif // METADEFINE_H
