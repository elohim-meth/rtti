﻿#ifndef METACONTAINER_H
#define METACONTAINER_H

#include "metaitem.h"

#include <functional>

namespace rtti {

class MetaContainer;
class MetaContainerPrivate;
class MetaNamespace;
class MetaClass;
class MetaConstructor;
class MetaMethod;
class MetaEnum;

struct DLL_PUBLIC IDefinitionCallbackHolder
{
    virtual void invoke(MetaContainer&) = 0;
    virtual ~IDefinitionCallbackHolder() noexcept = default;
};

class DLL_PUBLIC MetaContainer: public MetaItem
{
public:
    using enum_class_t = std::function<bool(const MetaClass*)>;
    using enum_method_t = std::function<bool(const MetaMethod*)>;

    const MetaNamespace* getNamespace(const char *name) const;
    std::size_t namespaceCount() const noexcept;
    const MetaNamespace* getNamespace(std::size_t index) const noexcept;

    const MetaClass* getClass(const char *name) const;
    std::size_t classCount() const noexcept;
    const MetaClass* getClass(std::size_t index) const noexcept;
    void for_each_class(const enum_class_t &func) const;

    const MetaMethod* getMethod(const char *name) const;
    const MetaMethod* getMethod(const std::string name) const;
    std::size_t methodCount() const noexcept;
    const MetaMethod* getMethod(std::size_t index) const noexcept;
    void for_each_method(const enum_method_t &func) const;

    const MetaConstructor* getConstructor(const char *name) const;
    std::size_t constructorCount() const noexcept;
    const MetaConstructor* defaultConstructor() const;
    const MetaConstructor* copyConstructor() const;
    const MetaConstructor* moveConstructor() const;

    const MetaEnum* getEnum(const char *name) const;
    std::size_t enumCount() const noexcept;

protected:
    explicit MetaContainer(const char *name, const MetaContainer &owner);
    explicit MetaContainer(MetaContainerPrivate &value);

    std::size_t count(MetaCategory category) const noexcept;
    const MetaItem* item(MetaCategory category, std::size_t index) const noexcept;
    const MetaItem* item(MetaCategory category, const char *name) const;

    bool addItem(MetaItem *value);
    void setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback);
    void checkDeferredDefine() const override;

private:
    DECLARE_PRIVATE(MetaContainer)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METACONTAINER_H

