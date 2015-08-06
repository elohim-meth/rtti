#ifndef METAPROPERTY_H
#define METAPROPERTY_H

#include "metaitem.h"
#include "metatype.h"
#include "argument.h"

namespace rtti {

struct DLL_PUBLIC IPropertyInvoker
{
    virtual bool isStatic() const = 0;
    virtual MetaType_ID typeId() const = 0;
    virtual bool readOnly() const = 0;
    virtual variant get_static() const = 0;
    virtual void set_static(argument arg) const = 0;
    virtual variant get_field(const variant &instance) const = 0;
    virtual void set_field(const variant &instance, argument arg) const = 0;
    virtual ~IPropertyInvoker() = default;
};

class MetaPropertyPrivate;

class DLL_PUBLIC MetaProperty final: public MetaItem
{
public:
    MetaCategory category() const override;

    template<typename ...Args>
    variant get(Args&&... args) const
    {
        using argument_indexes_t = index_sequence_for_t<Args...>;
        return get_selector(argument_indexes_t{}, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void set(Args&&... args) const
    {
        using argument_indexes_t = index_sequence_for_t<Args...>;
        set_selector(argument_indexes_t{}, std::forward<Args>(args)...);
    }

    bool readOnly() const
    {
        return invoker()->readOnly();
    }
protected:
    explicit MetaProperty(const char *name, MetaContainer &owner,
                        std::unique_ptr<IPropertyInvoker> invoker);
    static MetaProperty* create(const char *name, MetaContainer &owner,
                              std::unique_ptr<IPropertyInvoker> invoker);
private:
    const IPropertyInvoker* invoker() const;

    variant get_selector(index_sequence<>) const
    {
        auto interface = invoker();
        if (!interface->isStatic())
            throw invoke_error{"Trying to get static property " +
                               qualifiedName() + " as field property"};
        return interface->get_static();
    }

    variant get_selector(index_sequence<0>, const variant &instance) const
    {
        auto interface = invoker();
        if (interface->isStatic())
            throw invoke_error{"Trying to get field property " +
                               qualifiedName() + " as static property"};
        return interface->get_field(instance);
    }

    template<typename Arg>
    void set_selector(index_sequence<0>, Arg &&arg) const
    {
        auto interface = invoker();
        if (!interface->isStatic())
            throw invoke_error{"Trying to set static property " +
                               qualifiedName() + " as field property"};
        interface->set_static(std::forward<Arg>(arg));
    }

    template<typename Arg>
    void set_selector(index_sequence<0, 1>, const variant &instance, Arg &&arg) const
    {
        auto interface = invoker();
        if (interface->isStatic())
            throw invoke_error{"Trying to set field property " +
                               qualifiedName() + " as static property"};
        interface->set_field(instance, std::forward<Arg>(arg));
    }

    DECLARE_PRIVATE(MetaProperty)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METAPROPERTY_H

