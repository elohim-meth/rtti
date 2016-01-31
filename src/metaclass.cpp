#include "metaclass_p.h"
#include "metatype_p.h"
#include "metaerror.h"

#include <cassert>

namespace rtti {

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
                + " already registered as: " + metaClass->qualifiedName()};
    metaClass = this;
}

MetaClass* MetaClass::create(const char *name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaClass*>(owner.getClass(name));
    if (!result)
    {
        result = new MetaClass(name, owner, typeId);
        static_cast<internal::MetaContainerAccess&>(owner).addItem(result);
    }
    return result;
}

const MetaClass* MetaClass::findByTypeId(MetaType_ID typeId)
{
    auto type = MetaType{typeId};
    if (type.valid())
        return type.m_typeInfo->metaClass;
    return nullptr;
}

const MetaClass *MetaClass::findByTypeName(const char *name)
{
    if (!name)
        return nullptr;

    auto type = MetaType{name};
    if (type.valid())
        return type.m_typeInfo->metaClass;

    return nullptr;
}

MetaType_ID MetaClass::metaTypeId() const
{
    auto d = d_func();
    return d->m_typeId;
}

std::size_t MetaClass::baseClassCount() const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_baseClasses.count();
}

const MetaClass* MetaClass::baseClass(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    auto typeId = d->m_baseClasses.get(index);
    return findByTypeId(typeId);
}

std::size_t MetaClass::derivedClassCount() const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_derivedClasses.count();
}

const MetaClass *MetaClass::derivedClass(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    auto typeId = d->m_derivedClasses.get(index);
    return findByTypeId(typeId);
}

void MetaClass::addBaseClass(MetaType_ID typeId, cast_func_t caster)
{
    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};
    auto base = type.m_typeInfo->metaClass;
    if (!base)
        throw unregistered_metaclass{std::string{"Base class "}
                                     + type.typeName()
                                     +  " not registered"};

    auto d = d_func();
    d->m_baseClasses.add(typeId, caster);
    base->addDerivedClass(d->m_typeId);
}

void MetaClass::addDerivedClass(MetaType_ID typeId)
{
    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};
    auto derived = type.m_typeInfo->metaClass;
    if (!derived)
        throw unregistered_metaclass{std::string{"Derived class "}
                                     + type.typeName()
                                     +  "is not registered"};

    auto d = d_func();
    d->m_derivedClasses.add(typeId);
}

bool MetaClass::inheritedFrom(const MetaClass *base) const
{
    if (!base)
        return false;
    if (base == this)
        return true;

    checkDeferredDefine();
    auto d = d_func();
    auto result = false;
    d->m_baseClasses.for_each([&result, base](const internal::BaseClassList::item_t &item)
    {
        auto directBase = findByTypeId(item.first);
        assert(directBase);
        if (directBase->inheritedFrom(base))
        {
            result = true;
            return false;
        }
        return true;
    });
    return result;
}

const void* MetaClass::cast(const MetaClass *base, const void *instance) const
{
    if (!base)
        return nullptr;

    auto result = instance;
    if (base == this)
        return result;

    checkDeferredDefine();
    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, base, &found](const internal::BaseClassList::item_t &item)
    {
        auto directBase = findByTypeId(item.first);
        assert(directBase);
        if (directBase->inheritedFrom(base))
        {
            // cast to direct base
            result = directBase->cast(base, item.second(result));
            found = true;
            return false;
        }
        return true;
    });

    return (found ? result : nullptr);
}

void* MetaClass::cast(const MetaClass *base, void *instance) const
{
    return const_cast<void*>(cast(base, const_cast<const void*>(instance)));
}

const MetaMethod* MetaClass::getMethodInternal(const char *name) const
{
    using item_t = internal::BaseClassList::item_t;

    if (!name)
        return nullptr;
    auto result = MetaContainer::getMethodInternal(name);
    if (result)
        return result;

    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, name, &found](const item_t &item)
    {
        auto directBase = findByTypeId(item.first);
        assert(directBase);
        result = directBase->getMethodInternal(name);
        if (result)
        {
            found = true;
            return false;
        }
        return true;
    });
    return (found ? result : nullptr);
}

const MetaProperty* MetaClass::getPropertyInternal(const char *name) const
{
    using item_t = internal::BaseClassList::item_t;

    if (!name)
        return nullptr;
    auto result = MetaContainer::getPropertyInternal(name);
    if (result)
        return result;

    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, name, &found](const item_t &item)
    {
        auto directBase = findByTypeId(item.first);
        assert(directBase);
        result = directBase->getPropertyInternal(name);
        if (result)
        {
            found = true;
            return false;
        }
        return true;
    });
    return (found ? result : nullptr);
}

MetaCategory MetaClass::category() const
{
    return mcatClass;
}

} // namespace rtti
