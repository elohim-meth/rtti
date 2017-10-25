#ifndef METACONTAINER_H
#define METACONTAINER_H

#include "metatype.h"
#include "metaitem.h"
#include "signature.h"

#include <functional>
#include <vector>

namespace rtti {

class MetaContainer;
class MetaContainerPrivate;
class MetaNamespace;
class MetaClass;
class MetaConstructor;
class MetaMethod;
class MetaProperty;
class MetaEnum;

struct DLL_PUBLIC IDefinitionCallbackHolder
{
    virtual void invoke(MetaContainer&) = 0;
    virtual ~IDefinitionCallbackHolder() = default;
};

class DLL_PUBLIC MetaContainer: public MetaItem
{
    DECLARE_PRIVATE(MetaContainer)
public:
    enum class ForceDeferred
    {
        SelfOnly = 0,
        Recursive = 1
    };

    using enum_class_t = std::function<bool(MetaClass const*)>;
    using enum_method_t = std::function<bool(MetaMethod const*)>;

    MetaNamespace const* getNamespace(char const *name) const;
    std::size_t namespaceCount() const;
    MetaNamespace const* getNamespace(std::size_t index) const;

    MetaClass const* getClass(char const *name) const;
    std::size_t classCount() const;
    MetaClass const* getClass(std::size_t index) const;
    void for_each_class(enum_class_t const &func) const;

    MetaConstructor const* getConstructor(char const *name) const;
    MetaConstructor const* getConstructor(std::string const &name) const;
    template<typename ...Args>
    MetaConstructor const* getConstructor() const
    { return getConstructor(signature<Args...>::get("constructor")); }

    std::size_t constructorCount() const;
    MetaConstructor const* getConstructor(std::size_t index) const;
    MetaConstructor const* defaultConstructor() const;
    MetaConstructor const* copyConstructor() const;
    MetaConstructor const* moveConstructor() const;

    MetaMethod const* getMethod(char const *name) const;
    MetaMethod const* getMethod(std::string const &name) const;

    template<typename ...Args>
    MetaMethod const* getMethod(char const *name) const
    { return getMethod(signature<Args...>::get(name)); }
    template<typename ...Args>
    MetaMethod const* getMethod(std::string const &name) const
    { return getMethod(signature<Args...>::get(name.c_str())); }

    std::size_t methodCount() const;
    MetaMethod const* getMethod(std::size_t index) const;
    void for_each_method(enum_method_t const &func) const;

    MetaProperty const* getProperty(char const *name) const;
    std::size_t propertyCount() const;
    MetaProperty const* getProperty(std::size_t index) const;

    MetaEnum const* getEnum(char const *name) const;
    std::size_t enumCount() const;
    MetaEnum const* getEnum(std::size_t index) const;

    void forceDeferredDefine(ForceDeferred type = ForceDeferred::SelfOnly) const;
protected:
    explicit MetaContainer(std::string_view const &name, MetaContainer const &owner);
    explicit MetaContainer(MetaContainerPrivate &value);

    bool addItem(MetaItem *value);
    std::size_t count(MetaCategory category) const;
    MetaItem const* item(MetaCategory category, std::size_t index) const;
    MetaItem const* item(MetaCategory category, char const *name) const;

    void setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback);
    void checkDeferredDefine() const override;

    virtual MetaMethod const* getMethodInternal(char const *name) const;
    virtual MetaProperty const* getPropertyInternal(char const *name) const;

private:
    DECLARE_ACCESS_KEY(DeferredDefineKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    void setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback, DeferredDefineKey)
    { setDeferredDefine(std::move(callback)); }
};

} // namespace rtti

#endif // METACONTAINER_H

