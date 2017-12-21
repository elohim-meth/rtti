#include "metaclass_p.h"
#include "metatype_p.h"
#include "metaerror.h"

#include <cassert>

namespace rtti {

//--------------------------------------------------------------------------------------------------------------------------------
// MetaClass
//--------------------------------------------------------------------------------------------------------------------------------

MetaClass::MetaClass(std::string_view const &name, MetaContainer const &owner, MetaType_ID typeId)
    : MetaContainer{*new MetaClassPrivate{name, owner, typeId}}
{
    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};

    auto &metaClass = type.typeInfo({})->metaClass;
    if (metaClass)
        throw duplicate_metaclass{std::string{"Class "} + type.typeName()
                + " already registered as: " + metaClass->qualifiedName()};
    metaClass = this;
}

MetaClass* MetaClass::create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId)
{
    auto result = const_cast<MetaClass*>(owner.getClass(name));
    if (!result)
    {
        result = new MetaClass(name, owner, typeId);
        INVOKE_PROTECTED(owner, addItem, result);
    }
    return result;
}

MetaClass const* MetaClass::find(MetaType_ID typeId)
{
    auto type = MetaType{typeId};
    if (type.valid())
        return type.typeInfo({})->metaClass;
    return nullptr;
}

MetaClass const* MetaClass::find(std::string_view const &name)
{
    if (name.empty())
        return nullptr;

    auto type = MetaType{name};
    if (type.valid())
        return type.typeInfo({})->metaClass;

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

MetaClass const* MetaClass::baseClass(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    auto typeId = d->m_baseClasses.get(index);
    return find(typeId);
}

std::size_t MetaClass::derivedClassCount() const
{
    checkDeferredDefine();
    auto d = d_func();
    return d->m_derivedClasses.count();
}

MetaClass const* MetaClass::derivedClass(std::size_t index) const
{
    checkDeferredDefine();
    auto d = d_func();
    auto typeId = d->m_derivedClasses.get(index);
    return find(typeId);
}

void MetaClass::addBaseClass(MetaType_ID typeId, cast_func_t caster)
{
    auto type = MetaType{typeId};
    if (!type.valid())
        throw invalid_metatype_id{std::string{"Invalid MetaType_ID: "}
                                  + std::to_string(typeId.value())};
    auto base = type.typeInfo({})->metaClass;
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
    auto derived = type.typeInfo({})->metaClass;
    if (!derived)
        throw unregistered_metaclass{std::string{"Derived class "}
                                     + type.typeName()
                                     +  "is not registered"};

    auto d = d_func();
    d->m_derivedClasses.add(typeId);
}

bool MetaClass::inheritedFrom(MetaClass const *base) const
{
    if (!base)
        return false;
    if (base == this)
        return true;

    checkDeferredDefine();
    auto d = d_func();
    auto result = false;
    d->m_baseClasses.for_each([&result, base](auto const &item)
    {
        auto directBase = find(item.first);
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

void const* MetaClass::cast(MetaClass const *base, void const *instance) const
{
    if (!base)
        return nullptr;

    auto result = instance;
    if (base == this)
        return result;

    checkDeferredDefine();
    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, base, &found](auto const &item)
    {
        auto directBase = find(item.first);
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

void* MetaClass::cast(MetaClass const *base, void *instance) const
{
    return const_cast<void*>(cast(base, const_cast<void const*>(instance)));
}

MetaMethod const* MetaClass::getMethodInternal(std::string_view const &name) const
{
    using item_t = internal::BaseClassList::item_t;

    if (name.empty())
        return nullptr;

    auto result = MetaContainer::getMethodInternal(name);
    if (result)
        return result;

    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, &name, &found](item_t const &item)
    {
        auto directBase = find(item.first);
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

MetaProperty const* MetaClass::getPropertyInternal(std::string_view const &name) const
{
    using item_t = internal::BaseClassList::item_t;

    if (name.empty())
        return nullptr;

    auto result = MetaContainer::getPropertyInternal(name);
    if (result)
        return result;

    auto d = d_func();
    auto found = false;
    d->m_baseClasses.for_each([&result, &name, &found](item_t const &item)
    {
        auto directBase = find(item.first);
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
