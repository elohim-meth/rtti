#ifndef METADEFINE_H
#define METADEFINE_H

#include "metaenum.h"
#include "metaconstructor.h"
#include "metaclass.h"
#include "metanamespace.h"
#include "metacontainer.h"
#include "metaitem.h"

#include <typelist.h>

#include <sstream>
#include <stack>
#include <cassert>
#include <type_traits>

namespace rtti {


//forward
template<typename, typename> class meta_define;

namespace internal {

using container_stack_t = std::stack<MetaContainer*>;

template<typename DerivedType, typename BaseType>
static void* metacast_to_base(void* value)
{
    return static_cast<void*>(static_cast<BaseType*>(static_cast<DerivedType*>(value)));
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

template<typename C, typename ...Args>
struct ConstructorInvoker: IConstructorInvoker
{
    static_assert(sizeof...(Args) <= MaxNumberOfArguments, "Maximum supported arguments: 10");
    static_assert((sizeof...(Args) > 0) || std::is_default_constructible<C>::value,
                  "Type is not default constructible");
    static_assert((sizeof...(Args) == 0) || std::is_constructible<C, Args...>::value,
                  "Type can not be constructed with given arguments");

    template<std::size_t I>
    using argument_get_t = typename typelist_get<type_list<Args...>, I>::type;
    using argument_indexes_t = typename index_sequence_for<Args...>::type;
    using argument_array_t = std::array<const argument*, MaxNumberOfArguments>;

    variant invoke(argument arg0 = argument{},
                   argument arg1 = argument{},
                   argument arg2 = argument{},
                   argument arg3 = argument{},
                   argument arg4 = argument{},
                   argument arg5 = argument{},
                   argument arg6 = argument{},
                   argument arg7 = argument{},
                   argument arg8 = argument{},
                   argument arg9 = argument{}) const override
    {
        argument_array_t args = {
            &arg0, &arg1, &arg2, &arg3, &arg4,
            &arg5, &arg6, &arg7, &arg8, &arg9};

        auto count = sizeof...(Args);
        for (const auto &item: args)
        {
            if (item->empty())
                break;
            --count;
        }

        if (count != 0)
            throw invoke_error{"Invalid number of arguments"};

        return invoke_imp(args, argument_indexes_t{});
    }

    std::string signature() const override
    {
        return signature_imp(argument_indexes_t{});
    }

private:
    static constexpr const char* signature_imp(index_sequence<>)
    {
        return "default constructor";
    }

    template<std::size_t ...I>
    static std::string signature_imp(index_sequence<I...>)
    {
        constexpr auto size = sizeof...(I);
        if (size == 1)
        {
            using Arg = argument_get_t<0>;
            if (std::is_same<Arg, C>::value)
                return "copy constructor";

            using Decayed = typename std::decay<Arg>::type;
            if (std::is_same<Decayed, C>::value)
            {
                if (std::is_lvalue_reference<Arg>::value)
                    return "copy constructor";
                if (std::is_rvalue_reference<Arg>::value)
                    return "move constructor";
            }
        }

        std::ostringstream os;
        os << "constructor(";
        EXPAND (
            os << type_name<argument_get_t<I>>() << (I < size - 1 ? ", " : "")
        );
        os << ")";
        return os.str();
    }

    template<std::size_t ...I>
    static variant invoke_imp(const argument_array_t &args, index_sequence<I...>)
    {
        return C(args[I]->value<argument_get_t<I>>()...);
    }
};

} // namespace internal

template<typename T, typename MB = void>
class meta_define
{
public:
    using self_t = meta_define<T, MB>;

    template<typename V>
    self_t _attribute(const char *name, V &&value)
    {
        assert(m_currentItem);
        if (m_currentItem)
            m_currentItem->setAttribute(name, std::forward<V>(value));
        return std::move(*this);
    }

    template<typename V>
    self_t _attribute(const std::string &name, V &&value)
    {
        return _attribute(name.c_str(), std::forward<V>(value));
    }

    meta_define<void, self_t> _namespace(const char *name)
    {
        static_assert(std::is_same<T, void>::value, "Namespace can be defined only in another namespace");
        assert(m_currentContainer && m_currentContainer->category() == mcatNamespace && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaNamespace::create(name, *m_currentContainer);
        m_currentItem = m_currentContainer;
        return meta_define<void, self_t>{m_currentItem, m_currentContainer, m_containerStack};
    }

    meta_define<void, self_t> _namespace(const std::string &name)
    {
        return _namespace(name.c_str());
    }

    template<typename C>
    meta_define<C, self_t> _class(const char *name)
    {
        static_assert(std::is_class<C>::value, "Template argument <C> must be class");
        static_assert(!std::is_same<T, C>::value, "Recursive class definition");
        static_assert(std::is_same<T, void>::value || std::is_class<T>::value,
                      "Class can be defined in namespace or anther class");
        assert(m_currentContainer && m_containerStack);

        m_containerStack->push(m_currentContainer);
        m_currentContainer = MetaClass::create(name, *m_currentContainer, metaTypeId<C>());
        m_currentItem = m_currentContainer;

        meta_define<C, self_t> result {m_currentItem, m_currentContainer, m_containerStack};
        result.default_constructor_selector(std::is_default_constructible<C>{});
        result.copy_constructor_selector(std::is_copy_constructible<C>{});
        result.move_constructor_selector(std::is_move_constructible<C>{});
        return std::move(result);
    }

    template<typename C>
    meta_define<C, self_t> _class(const std::string &name)
    {
        return _class<C>(name.c_str());
    }

    template<typename C>
    meta_define<C, self_t> _class()
    {
        return _class<C>(type_name<C>());
    }

    template<typename F>
    self_t _lazy(F &&func)
    {
        static_assert(std::is_same<T, void>::value || std::is_class<T>::value,
                      "Deferred definition supported only for namespaces and class types");

        assert(m_currentContainer);
        using holder_t = internal::DefinitionCallbackHolder<T, typename std::decay<F>::type>;
        m_currentContainer->setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder>{
                                                  new holder_t{std::forward<F>(func)}});
        return std::move(*this);
    }

    template<typename ...B>
    self_t _base()
    {
        static_assert(std::is_class<T>::value, "Base classes can be defined only for class types");
        assert(m_currentContainer && m_currentContainer->category() == mcatClass);

        addBaseTypeList<type_list<B...>>(
                    static_cast<MetaClass*>(m_currentContainer),
                    typename index_sequence_for<B...>::type{});
        return std::move(*this);
    }

    template<typename E>
    self_t _enum(const char *name)
    {
        assert(m_currentContainer);
        m_currentItem = MetaEnum::create(name, *m_currentContainer, metaTypeId<E>());
        return std::move(*this);
    }

    template<typename E>
    self_t _enum(const std::string &name)
    {
        return _enum<E>(name.c_str());
    }

    template<typename V>
    self_t _element(const char *name, V &&value)
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
    self_t _element(const std::string &name, V &&value)
    {
        return _element(name.c_str(), std::forward<V>(value));
    }

    template<typename ...Args>
    self_t _constructor(const char *name = nullptr)
    {
        static_assert(std::is_class<T>::value, "Constructor can be defined only for class types");
        assert(m_currentContainer);
        auto category = m_currentContainer->category();
        assert(category == mcatClass);
        if (category == mcatClass)
        {
            MetaConstructor::create(name, *m_currentContainer,
                                    std::unique_ptr<IConstructorInvoker>{
                                        new internal::ConstructorInvoker<T, Args...>{}});
        }
        return std::move(*this);
    }

    MB _end()
    {
        static_assert(!std::is_same<MB, void>::value, "Container stack is EMPTY");
        assert(m_containerStack && !m_containerStack->empty());

        m_currentContainer = m_containerStack->top();
        m_currentItem = m_currentContainer;
        m_containerStack->pop();
        return MB{m_currentItem, m_currentContainer, m_containerStack};
    }

protected:
    meta_define() noexcept = default;
    meta_define(const meta_define&) = delete;
    meta_define& operator=(const meta_define&) = delete;
    meta_define(meta_define&&) = default;
    meta_define& operator=(meta_define&&) = default;

    meta_define(MetaItem *currentItem, MetaContainer *currentContainer, internal::container_stack_t *containerStack)
        : m_currentItem{currentItem}, m_currentContainer{currentContainer}, m_containerStack{containerStack}
    {}

    MetaItem *m_currentItem = nullptr;
    MetaContainer *m_currentContainer = nullptr;
    internal::container_stack_t *m_containerStack = nullptr;

private:
    template<typename C>
    struct check_is_class
    {
        static_assert(std::is_class<C>::value, "Type <C> is not a class");
        using type = C;
    };

    template<typename L, std::size_t ...I>
    static void addBaseTypeList(MetaClass *item, index_sequence<I...>)
    {
        // check that every type is class
        typename typelist_map<L, check_is_class>::type t __attribute__((unused));

        EXPAND(
            item->addBaseClass(metaTypeId<typename typelist_get<L, I>::type>(),
                               &internal::metacast_to_base<T, typename typelist_get<L, I>::type>)
        );
    }

    void default_constructor_selector(std::false_type)
    {}
    void default_constructor_selector(std::true_type)
    {
        _constructor();
    }

    void copy_constructor_selector(std::false_type)
    {}
    void copy_constructor_selector(std::true_type)
    {
        _constructor<const T&>();
    }

    void move_constructor_selector(std::false_type)
    {}
    void move_constructor_selector(std::true_type)
    {
        _constructor<T&&>();
    }

    template<typename> friend class internal::DefinitionCallbackHolder;
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

    friend meta_global global_define();
};

DLL_PUBLIC meta_global global_define();


} // namespace rtti

#endif // METADEFINE_H
