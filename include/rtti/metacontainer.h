#ifndef METACONTAINER_H
#define METACONTAINER_H

#include <rtti/metatype.h>
#include <rtti/metaitem.h>
#include <rtti/signature.h>

#include <functional>
#include <vector>

namespace rtti {

constexpr auto CONSTRUCTOR_SIG = "constructor";
constexpr auto DEFAULT_CONSTRUCTOR_SIG = "d_constructor";
constexpr auto COPY_CONSTRUCTOR_SIG = "c_constructor";
constexpr auto MOVE_CONSTRUCTOR_SIG = "m_constructor";

class MetaContainer;
class MetaContainerPrivate;
class MetaNamespace;
class MetaClass;
class MetaConstructor;
class MetaMethod;
class MetaProperty;
class MetaEnum;

struct RTTI_API IDefinitionCallbackHolder
{
    virtual void invoke(MetaContainer&) = 0;
    virtual ~IDefinitionCallbackHolder() = default;
};

class RTTI_API MetaContainer: public MetaItem
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

    MetaNamespace const* getNamespace(std::string_view name) const;
    std::size_t namespaceCount() const;
    MetaNamespace const* getNamespace(std::size_t index) const;

    MetaClass const* getClass(std::string_view name) const;
    std::size_t classCount() const;
    MetaClass const* getClass(std::size_t index) const;
    void for_each_class(enum_class_t const &func) const;

    MetaConstructor const* getConstructor(std::string_view name) const;
    template<typename ...Args>
    MetaConstructor const* getConstructor(std::string_view name = CONSTRUCTOR_SIG) const
    { return getConstructor(signature<Args...>::get(name)); }

    std::size_t constructorCount() const;
    MetaConstructor const* getConstructor(std::size_t index) const;
    MetaConstructor const* defaultConstructor() const;
    MetaConstructor const* copyConstructor() const;
    MetaConstructor const* moveConstructor() const;

    MetaMethod const* getMethod(std::string_view name) const;

    template<typename ...Args>
    MetaMethod const* getMethod(std::string_view name) const
    { return getMethod(signature<Args...>::get(name)); }

    std::size_t methodCount() const;
    MetaMethod const* getMethod(std::size_t index) const;
    void for_each_method(enum_method_t const &func) const;

    MetaProperty const* getProperty(std::string_view name) const;
    std::size_t propertyCount() const;
    MetaProperty const* getProperty(std::size_t index) const;

    MetaEnum const* getEnum(std::string_view name) const;
    std::size_t enumCount() const;
    MetaEnum const* getEnum(std::size_t index) const;

    void forceDeferredDefine(ForceDeferred type = ForceDeferred::SelfOnly) const;
protected:
    RTTI_PRIVATE explicit MetaContainer(std::string_view name, MetaContainer const &owner);
    RTTI_PRIVATE explicit MetaContainer(MetaContainerPrivate &value);

    RTTI_PRIVATE bool addItem(MetaItem *value);
    RTTI_PRIVATE std::size_t count(MetaCategory category) const;
    RTTI_PRIVATE MetaItem const* item(MetaCategory category, std::size_t index) const;
    RTTI_PRIVATE MetaItem const* item(MetaCategory category, std::string_view name) const;

    void setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback);
    RTTI_PRIVATE void checkDeferredDefine() const override;

    RTTI_PRIVATE virtual MetaMethod const* getMethodInternal(std::string_view name) const;
    RTTI_PRIVATE virtual MetaProperty const* getPropertyInternal(std::string_view name) const;

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

