#include "metaitem.h"
#include "metatype_p.h"
#include "variant.h"
#include "metadefine.h"
#include "metaerror.h"

#include <finally.h>

#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <cassert>

namespace rtti {

namespace {

const std::string empty_string = {};

struct CStringCompare {
    bool operator()(const char *lhs, const char *rhs) const
    { return std::strcmp(lhs, rhs) < 0; }
};

}

//--------------------------------------------------------------------------------------------------------------------------------
// AttributeList
//--------------------------------------------------------------------------------------------------------------------------------

struct DLL_LOCAL NamedVariant
{
    template<typename T>
    NamedVariant(const char *name, T&& value)
        : name{name},
          value{std::forward<T>(value)}
    {}

    const std::string name;
    variant value;
};

class DLL_LOCAL NamedVariantList
{
public:
    template<typename T>
    void set(const char *name, T &&value);
    const variant& get(std::size_t index) const noexcept;
    const variant& get(const char *name) const;
    const std::string& name(std::size_t index) const noexcept;
    std::size_t size() const noexcept
    {
        return m_items.size();
    }
    auto begin() const noexcept -> std::vector<NamedVariant>::const_iterator
    {
        return m_items.begin();
    }
    auto end() const noexcept -> std::vector<NamedVariant>::const_iterator
    {
        return m_items.end();
    }

private:
    std::vector<NamedVariant> m_items;
    std::map<const char*, std::size_t, CStringCompare> m_names;
};

template<typename T>
inline void NamedVariantList::set(const char *name, T &&value)
{
    if (strnlen(name, 1) == 0)
        return;

    auto search = m_names.find(name);

    if (search == std::end(m_names)) {
        auto index = m_items.size();
        m_items.emplace_back(name, std::forward<T>(value));
        const auto &item = m_items.back();
        m_names.emplace(item.name.c_str(), index);
    }
    else {
        auto index = search->second;
        m_items.at(index).value = std::forward<T>(value);
    }
}

inline const variant& NamedVariantList::get(std::size_t index) const noexcept
{
    if (index < m_items.size())
        return m_items[index].value;
    return variant::empty_variant;
}

inline const variant& NamedVariantList::get(const char *name) const
{
    if (strnlen(name, 1) > 0)
    {
        auto search = m_names.find(name);
        if (search != std::end(m_names)) {
            auto index = search->second;
            return get(index);
        }
    }
    return variant::empty_variant;
}

inline const std::string& NamedVariantList::name(std::size_t index) const noexcept
{
    if (index < m_items.size())
        return m_items[index].name;
    return empty_string;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItemPrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaItemPrivate
{
public:
    MetaItemPrivate() = delete;
    MetaItemPrivate(const MetaItemPrivate&) = delete;
    MetaItemPrivate& operator=(const MetaItemPrivate&) = delete;
    MetaItemPrivate(MetaItemPrivate&&) = delete;
    MetaItemPrivate& operator=(MetaItemPrivate&&) = delete;

    // Constructor for global namespace
    MetaItemPrivate(const char *name)
        : m_name(name)
    {}

    MetaItemPrivate(const char *name, const MetaContainer &owner)
        : m_name(name), m_owner(&owner)
    {}
    virtual ~MetaItemPrivate() noexcept = default;

protected:
    const std::string& name() const
    { return m_name; }
    const MetaContainer* owner() const
    { return m_owner; }
    const std::string& qualifiedName() const;

private:
    std::string makeQualifiedName() const;

    const std::string m_name;
    const MetaContainer *m_owner = nullptr;
    NamedVariantList m_attributes;
    mutable std::string m_qualifiedName;

    friend class MetaItem;
};

inline std::string MetaItemPrivate::makeQualifiedName() const
{
    std::string result = m_owner ? m_owner->qualifiedName() : "";
    return std::move(result) + (m_owner ? "::" + m_name : "");
}

inline const std::string& MetaItemPrivate::qualifiedName() const
{
    if (m_qualifiedName.empty())
        m_qualifiedName = makeQualifiedName();
    return m_qualifiedName;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItemList
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaItemList
{
public:
    // Need custom deleter since MetaItem destructor is protected
    struct MetaItemDeleter {
        void operator()(MetaItem *value) const
        { delete value; }
    };

    using item_t = std::unique_ptr<MetaItem, MetaItemDeleter>;

    bool add(MetaItem *value);
    MetaItem* get(std::size_t index) const noexcept;
    MetaItem* get(const char *name) const;
    std::size_t size() const noexcept
    {
        return m_items.size();
    }
    auto begin() const noexcept -> std::vector<item_t>::const_iterator
    {
        return m_items.begin();
    }
    auto end() const noexcept -> std::vector<item_t>::const_iterator
    {
        return m_items.end();
    }
private:
    std::vector<item_t> m_items;
    std::map<const char *, std::size_t, CStringCompare> m_names;
};

bool MetaItemList::add(MetaItem *value)
{
    if (!value)
        return false;

    auto itemFound = (std::find_if(
                          std::begin(m_items),
                          std::end(m_items),
                          [value] (const item_t &item) {
                              return value == item.get();
                          }) != std::end(m_items));
    auto nameFound = ((m_names.find(value->name().c_str())) != std::end(m_names));

    if (!itemFound && !nameFound) {
        auto index = m_items.size();
        m_items.emplace_back(value);
        m_names.emplace(value->name().c_str(), index);
        return true;
    }
    return false;
}

inline MetaItem* MetaItemList::get(std::size_t index) const noexcept
{
    MetaItem *result = nullptr;
    if (index < m_items.size())
        result = m_items[index].get();
    return result;
}

inline MetaItem* MetaItemList::get(const char *name) const
{
    MetaItem *result = nullptr;
    if (strnlen(name, 1) > 0)
    {
        auto it = m_names.find(name);
        if (it != std::end(m_names)) {
            auto index = it->second;
            result = get(index);
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainerPrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaContainerPrivate: public MetaItemPrivate
{
public:
    using MetaItemPrivate::MetaItemPrivate;

    MetaItem* item(MetaCategory category, std::size_t index) const noexcept
    { return m_lists[category]->get(index); }
    std::size_t size(MetaCategory category) const noexcept
    { return m_lists[category]->size(); }
    MetaItem* item(MetaCategory category, const char *name) const
    { return m_lists[category]->get(name); }

protected:
    bool addItem(MetaItem *value);

private:
    MetaItemList m_namespaces;
    MetaItemList m_classes;
    MetaItemList m_properties;
    MetaItemList m_methods;
    MetaItemList m_enums;
    MetaItemList m_constructors;
    const std::array<MetaItemList*, mcatCount> m_lists = {
        {&m_namespaces, &m_classes, &m_properties,
         &m_methods, &m_enums, &m_constructors}
    };

    mutable bool m_deferredDefine_lock = false;
    mutable std::unique_ptr<IDefinitionCallbackHolder> m_deferredDefine;

    friend class MetaContainer;
};

inline bool MetaContainerPrivate::addItem(MetaItem *value)
{
    if (!value)
        return false;

    auto category = value->category();
    return m_lists[category]->add(value);
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaNamespacePrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaNamespacePrivate: public MetaContainerPrivate
{
public:
    using MetaContainerPrivate::MetaContainerPrivate;
private:
    friend class MetaNamespace;
};

//--------------------------------------------------------------------------------------------------------------------------------
// MetaClassPrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaTypeIDs
{
public:
    void add(MetaType_ID value) noexcept
    {
        if (!find(value))
            m_items.push_back(value);
    }
    std::size_t count() const noexcept
    {
        return m_items.size();
    }
    MetaType_ID get(std::size_t index) const noexcept
    {
        if (index < m_items.size())
            return m_items[index];
        return MetaType_ID{};
    }

    bool find(MetaType_ID value) const
    {
        auto search = std::find(std::begin(m_items), std::end(m_items), value);
        return (search != std::end(m_items));
    }
private:
    std::vector<MetaType_ID> m_items;
};

class DLL_LOCAL MetaClassPrivate: public MetaContainerPrivate
{
public:
    MetaClassPrivate(const char *name, const MetaContainer &owner, MetaType_ID typeId)
        : MetaContainerPrivate{name, owner}, m_typeId{typeId}
    {}
private:
    MetaType_ID m_typeId;
    bool m_addBase_lock = false;
    bool m_addDerived_lock = false;
    MetaTypeIDs m_baseClasses;
    MetaTypeIDs m_derivedClasses;

    friend class MetaClass;
};

//--------------------------------------------------------------------------------------------------------------------------------
// MetaConstructorPrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaConstructorPrivate: public MetaItemPrivate
{
public:
    MetaConstructorPrivate(const char *name, const MetaContainer &owner,
                           std::unique_ptr<IConstructorInvoker> constructor)
        : MetaItemPrivate{name, owner}, m_constructor{std::move(constructor)}
    {}

private:
    std::unique_ptr<IConstructorInvoker> m_constructor;
    friend class MetaConstructor;
};

//--------------------------------------------------------------------------------------------------------------------------------
// MetaEnumPrivate
//--------------------------------------------------------------------------------------------------------------------------------

class DLL_LOCAL MetaEnumPrivate: public MetaItemPrivate
{
public:
    MetaEnumPrivate(const char *name, const MetaContainer &owner, MetaType_ID typeId)
        : MetaItemPrivate{name, owner}, m_typeId{typeId}
    {}

private:
    MetaType_ID m_typeId;
    NamedVariantList m_elements;

    friend class MetaEnum;
};

//--------------------------------------------------------------------------------------------------------------------------------
// MetaItem
//--------------------------------------------------------------------------------------------------------------------------------

MetaItem::MetaItem(const char *name, const MetaContainer &owner)
    : d_ptr(new MetaItemPrivate(name, owner))
{}

MetaItem::MetaItem(MetaItemPrivate &value)
    : d_ptr(&value)
{}

MetaItem::~MetaItem()
{}

void MetaItem::checkDeferredDefine() const
{}

const std::string& MetaItem::name() const
{
    const auto d = d_func();
    return d->name();
}

const MetaContainer *MetaItem::owner() const noexcept
{
    const auto d = d_func();
    return d->owner();
}

const std::string& MetaItem::qualifiedName() const
{
    const auto d = d_func();
    return d->qualifiedName();
}

std::size_t MetaItem::attributeCount() const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_attributes.size();
}

const variant& MetaItem::attribute(std::size_t index) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_attributes.get(index);
}

const std::string &MetaItem::attributeName(std::size_t index) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_attributes.name(index);
}

const variant& MetaItem::attribute(const char *name) const
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_attributes.get(name);
}

void MetaItem::for_each_attribute(const std::function<void(const std::string&, const variant&)> &func) const
{
    if (!func)
        return;

    const auto d = d_func();
    for (const auto &item: d->m_attributes)
        func(item.name, item.value);
}

void MetaItem::setAttribute(const char *name, const variant &value)
{
    auto d = d_func();
    d->m_attributes.set(name, value);
}

void MetaItem::setAttribute(const char *name, variant &&value)
{
    auto d = d_func();
    d->m_attributes.set(name, std::move(value));
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainerAccess
//--------------------------------------------------------------------------------------------------------------------------------

namespace {
struct MetaContainerAccess: MetaContainer
{
    bool addItem(MetaItem *value)
    { return MetaContainer::addItem(value); }

    friend class MetaNamespace;
    friend class MetaClass;
    friend class MetaEnum;
    friend class MetaProperty;
    friend class MetaConstructor;
    friend class MetaMethod;
};
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaContainer
//--------------------------------------------------------------------------------------------------------------------------------

MetaContainer::MetaContainer(const char *name, const MetaContainer &owner)
    : MetaItem(*new MetaContainerPrivate(name, owner))
{}

MetaContainer::MetaContainer(MetaContainerPrivate &value)
    : MetaItem(value)
{}

bool MetaContainer::addItem(MetaItem *value)
{
    auto d = d_func();
    return d->addItem(value);
}

void MetaContainer::setDeferredDefine(std::unique_ptr<IDefinitionCallbackHolder> callback)
{
    auto d = d_func();
    assert(!d->m_deferredDefine);
    if (d->m_deferredDefine)
        return;
    d->m_deferredDefine = std::move(callback);
}

void MetaContainer::checkDeferredDefine() const
{
    auto d = d_func();
    if (d->m_deferredDefine_lock || !d->m_deferredDefine)
        return;

    d->m_deferredDefine_lock = true;
    FINALLY{
        d->m_deferredDefine_lock = false;
        d->m_deferredDefine = nullptr;
    };

    d->m_deferredDefine->invoke(*const_cast<MetaContainer*>(this));
}

std::size_t MetaContainer::count(MetaCategory category) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->size(category);
}

const MetaItem* MetaContainer::item(MetaCategory category, const char *name) const
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->item(category, name);
}

const MetaItem* MetaContainer::item(MetaCategory category, std::size_t index) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->item(category, index);
}

const MetaNamespace* MetaContainer::getNamespace(const char *name) const
{
    return static_cast<const MetaNamespace*>(item(mcatNamespace, name));
}

std::size_t MetaContainer::namespaceCount() const noexcept
{
    return count(mcatNamespace);
}

const MetaClass* MetaContainer::getClass(const char *name) const
{
    return static_cast<const MetaClass*>(item(mcatClass, name));
}

std::size_t MetaContainer::classCount() const noexcept
{
    return count(mcatClass);
}

const MetaConstructor* MetaContainer::getConstructor(const char *name) const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, name));
}

std::size_t MetaContainer::constructorCount() const noexcept
{
    return count(mcatConstructor);
}

const MetaConstructor *MetaContainer::defaultConstructor() const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, "default constructor"));
}

const MetaConstructor *MetaContainer::copyConstructor() const
{
    return static_cast<const MetaConstructor*>(item(mcatConstructor, "copy constructor"));
}

const MetaConstructor *MetaContainer::moveConstructor() const
{

    return static_cast<const MetaConstructor*>(item(mcatConstructor, "move constructor"));
}

void MetaContainer::for_each_class(std::function<void (const MetaClass *)> &func) const
{
    if (!func)
        return;
    const auto d = d_func();
    for(const auto &item: d->m_classes)
    {
        func(static_cast<MetaClass*>(item.get()));
    }
}

const MetaEnum *MetaContainer::getEnum(const char *name) const
{
    return static_cast<const MetaEnum*>(item(mcatEnum, name));
}

std::size_t MetaContainer::enumCount() const noexcept
{
    return count(mcatEnum);
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaNamespace
//--------------------------------------------------------------------------------------------------------------------------------

MetaNamespace::MetaNamespace()
    : MetaContainer(*new MetaNamespacePrivate{"global"})
{}

const MetaNamespace* MetaNamespace::global() noexcept
{
    static MetaNamespace globalNamespace;
    return &globalNamespace;
}

bool MetaNamespace::isGlobal() const noexcept
{
    const auto d = d_func();
    return (d->owner() == nullptr);
}

MetaNamespace* MetaNamespace::create(const char *name, MetaContainer &owner)
{
    auto result = const_cast<MetaNamespace*>(owner.getNamespace(name));
    if (!result)
    {
        result = new MetaNamespace(name, owner);
        static_cast<MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

MetaCategory MetaNamespace::category() const noexcept
{
    return mcatNamespace;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaClass
//--------------------------------------------------------------------------------------------------------------------------------

MetaClass::MetaClass(const char *name, const MetaContainer &owner, MetaType_ID typeId)
    : MetaContainer{*new MetaClassPrivate{name, owner, typeId}}
{
    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};

    auto &metaClass = const_cast<TypeInfo*>(type.m_typeInfo)->metaClass;
    if (metaClass)
        throw duplicate_metaclass{std::string{"Class "} + type.typeName()
                + " already registered with name: " + metaClass->name()};
    metaClass = this;
}

MetaClass* MetaClass::create(const char *name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaClass*>(owner.getClass(name));
    if (!result)
    {
        result = new MetaClass(name, owner, typeId);
        static_cast<MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

const MetaClass* MetaClass::findByTypeId(MetaType_ID typeId) noexcept
{
    MetaClass *result = nullptr;
    auto type = MetaType{typeId};
    if (type.valid())
        result = type.m_typeInfo->metaClass;
    return result;
}

MetaType_ID MetaClass::metaTypeId() const noexcept
{
    const auto d = d_func();
    return d->m_typeId;
}

std::size_t MetaClass::baseClassCount() const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_baseClasses.count();
}

const MetaClass* MetaClass::baseClass(std::size_t index) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    auto typeId = d->m_baseClasses.get(index);
    return findByTypeId(typeId);
}

std::size_t MetaClass::derivedClassCount() const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    return d->m_derivedClasses.count();
}

const MetaClass *MetaClass::derivedClass(std::size_t index) const noexcept
{
    checkDeferredDefine();
    const auto d = d_func();
    auto typeId = d->m_derivedClasses.get(index);
    return findByTypeId(typeId);
}

void MetaClass::addBaseClass(MetaType_ID typeId)
{
    auto d = d_func();
    if (d->m_addBase_lock)
        return;

    d->m_addBase_lock = true;
    FINALLY { d->m_addBase_lock = false; };

    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};
    auto base = type.m_typeInfo->metaClass;
    if (!base)
        throw unregistered_metaclass{std::string{"Base class "}
                                     + type.typeName()
                                     +  " not registered"};

    d->m_baseClasses.add(typeId);
    base->addDerivedClass(d->m_typeId);
}

void MetaClass::addDerivedClass(MetaType_ID typeId)
{
    auto d = d_func();
    if (d->m_addDerived_lock)
        return;

    d->m_addDerived_lock = true;
    FINALLY { d->m_addDerived_lock = false; };

    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};
    auto derived = type.m_typeInfo->metaClass;
    if (!derived)
        throw unregistered_metaclass{std::string{"Derived class "}
                                     + type.typeName()
                                     +  "is not registered"};

    d->m_derivedClasses.add(typeId);
    derived->addBaseClass(d->m_typeId);
}

bool MetaClass::inheritedFrom(const MetaClass *base) const noexcept
{
    if (!base)
        return false;
    auto baseId = base->metaTypeId();
    const auto d = d_func();
    return d->m_baseClasses.find(baseId);
}

MetaCategory MetaClass::category() const noexcept
{
    return mcatClass;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaConstructor
//--------------------------------------------------------------------------------------------------------------------------------

MetaConstructor::MetaConstructor(const char *name, MetaContainer &owner,
                                 std::unique_ptr<IConstructorInvoker> constructor)
    : MetaItem{*new MetaConstructorPrivate{name, owner, std::move(constructor)}}
{}

MetaConstructor* MetaConstructor::create(const char *name, MetaContainer &owner,
                                         std::unique_ptr<IConstructorInvoker> constructor)
{
    if (!constructor)
        return nullptr;
    auto category = owner.category();
    if (category != mcatClass)
        throw invalid_meta_define{"Constructor can be defined only for class types"};

    if (!name || strnlen(name, 1) == 0)
        name = constructor->signature();

    auto result = const_cast<MetaConstructor*>(owner.getConstructor(name));
    if (!result)
    {
        result = new MetaConstructor(name, owner, std::move(constructor));
        static_cast<MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

const IConstructorInvoker* MetaConstructor::constructor() const noexcept
{
    const auto d = d_func();
    return d->m_constructor.get();
}

MetaCategory MetaConstructor::category() const noexcept
{
    return mcatConstructor;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaProperty
//--------------------------------------------------------------------------------------------------------------------------------

MetaCategory MetaProperty::category() const noexcept
{
    return mcatProperty;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaMethod
//--------------------------------------------------------------------------------------------------------------------------------

MetaCategory MetaMethod::category() const noexcept
{
    return mcatMethod;
}

//--------------------------------------------------------------------------------------------------------------------------------
// MetaEnum
//--------------------------------------------------------------------------------------------------------------------------------

MetaEnum::MetaEnum(const char *name, const MetaContainer &owner, MetaType_ID typeId)
    : MetaItem{*new MetaEnumPrivate{name, owner, typeId}}
{}

MetaEnum *MetaEnum::create(const char *name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaEnum*>(owner.getEnum(name));
    if (!result)
    {
        result = new MetaEnum(name, owner, typeId);
        static_cast<MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

void MetaEnum::addElement(const char *name, variant &&value)
{
    auto d = d_func();
    d->m_elements.set(name, std::move(value));
}

MetaCategory MetaEnum::category() const noexcept
{
    return mcatEnum;
}

std::size_t MetaEnum::elementCount() const noexcept
{
    const auto d = d_func();
    return d->m_elements.size();
}

const variant& MetaEnum::element(std::size_t index) const noexcept
{
    const auto d = d_func();
    return d->m_elements.get(index);
}

const std::string &MetaEnum::elementName(std::size_t index) const noexcept
{
    const auto d = d_func();
    return d->m_elements.name(index);
}

const variant& MetaEnum::element(const char *name) const
{
    const auto d = d_func();
    return d->m_elements.get(name);
}

void MetaEnum::for_each_element(const std::function<void (const std::string &, const variant &)> &func) const
{
    if (!func)
        return;
    const auto d = d_func();
    for (const auto &item: d->m_elements)
        func(item.name, item.value);
}

} // namespace rtti
