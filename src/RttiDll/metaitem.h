#ifndef METAITEM_H
#define METAITEM_H

#include "metatype.h"
#include "argument.h"

#include <typelist.h>

#include <memory>

#include "global.h"

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr.get()); } \
    friend class rtti::Class##Private;

namespace rtti {

enum MetaCategory: int {
    mcatNamespace,
    mcatClass,
    mcatProperty,
    mcatMethod,
    mcatEnum,
    mcatConstructor,
    mcatCount
};

// forward
class variant;
class MetaItem;
class MetaItemList;
class MetaItemPrivate;
class MetaContainer;
class MetaContainerPrivate;
class MetaNamespace;
class MetaNamespacePrivate;
class MetaClass;
class MetaClassPrivate;
class MetaConstructor;
class MetaConstructorPrivate;
class MetaEnum;
class MetaEnumPrivate;
template<typename, typename> class meta_define;

class DLL_PUBLIC MetaItem
{
public:
    virtual MetaCategory category() const noexcept = 0;
    const std::string& name() const;
    const MetaContainer* owner() const noexcept;
    const std::string& qualifiedName() const;
    std::size_t attributeCount() const noexcept;
    const variant& attribute(std::size_t index) const noexcept;
    const std::string& attributeName(std::size_t index) const noexcept;
    const variant& attribute(const char *name) const;
    void for_each_attribute(const std::function<void(const std::string&, const variant&)> &func) const;
protected:
    explicit MetaItem(const char *name, const MetaContainer &owner);
    explicit MetaItem(MetaItemPrivate &value);
    MetaItem(const MetaItem &) = delete;
    MetaItem& operator=(const MetaItem &) = delete;
    MetaItem(MetaItem &&) = delete;
    MetaItem& operator=(MetaItem &&) = delete;
    virtual ~MetaItem();

    virtual void checkDeferredDefine() const;

    void setAttribute(const char *name, const variant &value);
    void setAttribute(const char *name, variant &&value);
    std::unique_ptr<MetaItemPrivate> d_ptr;

private:
    DECLARE_PRIVATE(MetaItem)
    friend class rtti::MetaItemList;
    template<typename, typename> friend class rtti::meta_define;
};

struct DLL_PUBLIC IDefinitionCallbackHolder
{
    virtual void invoke(MetaContainer&) = 0;
    virtual ~IDefinitionCallbackHolder() noexcept = default;
};

class DLL_PUBLIC MetaContainer: public MetaItem
{
public:
    std::size_t count(MetaCategory category) const noexcept;
    const MetaItem* item(MetaCategory category, std::size_t index) const noexcept;
    const MetaItem* item(MetaCategory category, const char *name) const;

    const MetaNamespace* getNamespace(const char *name) const;
    std::size_t namespaceCount() const noexcept;

    const MetaClass* getClass(const char *name) const;
    std::size_t classCount() const noexcept;
    void for_each_class(std::function<void(const MetaClass*)> &func) const;

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

    bool addItem(MetaItem *value);
    void setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback);
    void checkDeferredDefine() const override;

private:
    DECLARE_PRIVATE(MetaContainer)
    template<typename, typename> friend class rtti::meta_define;
};

class DLL_PUBLIC MetaNamespace final: public MetaContainer
{
public:
    static const MetaNamespace* global() noexcept;
    bool isGlobal() const noexcept;
    MetaCategory category() const noexcept override;
protected:
    using MetaContainer::MetaContainer;
    static MetaNamespace* create(const char *name, MetaContainer &owner);
private:
    MetaNamespace(); //global namespace

    DECLARE_PRIVATE(MetaNamespace)
    template<typename, typename> friend class rtti::meta_define;
};

class DLL_PUBLIC MetaClass final: public MetaContainer
{
public:
    MetaCategory category() const noexcept override;
    static const MetaClass* findByTypeId(MetaType_ID typeId) noexcept;
    static const MetaClass* findByTypeName(const char *name);
    MetaType_ID metaTypeId() const noexcept;
    std::size_t baseClassCount() const noexcept;
    const MetaClass* baseClass(std::size_t index) const noexcept;
    std::size_t derivedClassCount() const noexcept;
    const MetaClass* derivedClass(std::size_t index) const noexcept;
    bool inheritedFrom(const MetaClass *base) const noexcept;
protected:
    explicit MetaClass(const char *name, const MetaContainer &owner, MetaType_ID typeId);
    static MetaClass* create(const char *name, MetaContainer &owner, MetaType_ID typeId);

    void addBaseClass(MetaType_ID typeId);
    void addDerivedClass(MetaType_ID typeId);
private:
    DECLARE_PRIVATE(MetaClass)
    template<typename, typename> friend class rtti::meta_define;
};

struct DLL_PUBLIC IConstructorInvoker
{
    enum {
        MaxNumberOfArguments = 10
    };

    virtual variant invoke(argument arg0 = argument{},
                           argument arg1 = argument{},
                           argument arg2 = argument{},
                           argument arg3 = argument{},
                           argument arg4 = argument{},
                           argument arg5 = argument{},
                           argument arg6 = argument{},
                           argument arg7 = argument{},
                           argument arg8 = argument{},
                           argument arg9 = argument{}) const = 0;
    virtual std::string signature() const = 0;
    virtual ~IConstructorInvoker() noexcept = default;
};

class DLL_PUBLIC MetaConstructor final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
    template<typename ...Args>
    variant invoke(Args&&... args) const
    {
        static_assert(sizeof...(Args) <= IConstructorInvoker::MaxNumberOfArguments,
                      "Maximum supported metaconstructor arguments: 10");
        return constructor()->invoke(std::forward<Args>(args)...);
    }

protected:
    explicit MetaConstructor(std::string &&name, MetaContainer &owner,
                             std::unique_ptr<IConstructorInvoker> constructor);
    static MetaConstructor* create(const char *name, MetaContainer &owner,
                                   std::unique_ptr<IConstructorInvoker> constructor);

private:
    const IConstructorInvoker* constructor() const noexcept;

    DECLARE_PRIVATE(MetaConstructor)
    template<typename, typename> friend class rtti::meta_define;
};

class DLL_PUBLIC MetaEnum final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;

    std::size_t elementCount() const noexcept;
    const variant& element(std::size_t index) const noexcept;
    const std::string& elementName(std::size_t index) const noexcept;
    const variant& element(const char *name) const;
    void for_each_element(const std::function<void(const std::string&, const variant&)> &func) const;
protected:
    explicit MetaEnum(const char *name, const MetaContainer &owner, MetaType_ID typeId);
    static MetaEnum* create(const char *name, MetaContainer &owner, MetaType_ID typeId);

    void addElement(const char *name, variant &&value);
private:
    DECLARE_PRIVATE(MetaEnum)
    template<typename, typename> friend class rtti::meta_define;
};

class DLL_PUBLIC MetaProperty final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
};

class DLL_PUBLIC MetaMethod final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
};

} //namespace rtti

#endif // METAITEM_H
