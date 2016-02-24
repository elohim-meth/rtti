#ifndef METACLASS_H
#define METACLASS_H

#include "metacontainer.h"
#include "metatype.h"
#include "metaerror.h"

#include <sfinae.h>

namespace rtti {

namespace internal {
template<typename To, typename From>
To const* meta_cast(const From*, std::true_type);
} // namespace internal

class MetaClassPrivate;

class DLL_PUBLIC MetaClass final: public MetaContainer
{
public:
    using cast_func_t = const void*(*)(const void*);

    MetaCategory category() const override;
    static const MetaClass* findByTypeId(MetaType_ID typeId);
    static const MetaClass* findByTypeName(const char *name);
    MetaType_ID metaTypeId() const;
    std::size_t baseClassCount() const;
    const MetaClass* baseClass(std::size_t index) const;
    std::size_t derivedClassCount() const;
    const MetaClass* derivedClass(std::size_t index) const;
    bool inheritedFrom(const MetaClass *base) const;
protected:
    explicit MetaClass(const char *name, const MetaContainer &owner, MetaType_ID typeId);
    static MetaClass* create(const char *name, MetaContainer &owner, MetaType_ID typeId);

    void addBaseClass(MetaType_ID typeId, cast_func_t caster);
    void addDerivedClass(MetaType_ID typeId);
    const void* cast(const MetaClass *base, const void *instance) const;
    void* cast(const MetaClass *base, void *instance) const;

    const MetaMethod* getMethodInternal(const char *name) const override;
    const MetaProperty* getPropertyInternal(const char *name) const override;
private:
    DECLARE_PRIVATE(MetaClass)
    template<typename, typename> friend class rtti::meta_define;
    template<typename To, typename From>
    friend const To* internal::meta_cast(const From*, std::true_type);
    friend class rtti::variant;
};

struct DLL_PUBLIC ClassInfo
{
    MetaType_ID typeId;
    const void *instance = nullptr;

    constexpr ClassInfo() = default;
    constexpr ClassInfo(MetaType_ID typeId, const void *instance)
        : typeId(typeId), instance(instance)
    {}
};

#define DECLARE_CLASSINFO \
public: \
    virtual rtti::ClassInfo classInfo() const \
    { \
        return {rtti::metaTypeId<typename std::decay<decltype(*this)>::type>(), this}; \
    } \
private: \

namespace internal {

template<typename To, typename From>
const To* meta_cast(const From *from, std::true_type)
{
    static_assert(std::is_class<From>::value && std::is_class<To>::value,
                  "Both template arguments should be classes");
    if (!from)
        return nullptr;

    const auto &info = from->classInfo();
    auto fromClass = MetaClass::findByTypeId(info.typeId);
    auto toClass = MetaClass::findByTypeId(metaTypeId<To>());
    if (!fromClass || !toClass)
        return nullptr;

    auto result = fromClass->cast(toClass, info.instance);
    if (!result)
        return nullptr;
    return static_cast<const To*>(result);
}

template<typename To, typename From>
To* meta_cast(From *from, std::true_type)
{
    return const_cast<To*>(meta_cast<To>(const_cast<const From*>(from), std::true_type{}));
}

template<typename To, typename From>
const To* meta_cast(const From *from, std::false_type)
{
    return from;
}

template<typename To, typename From>
To* meta_cast(From *from, std::false_type)
{
    return from;
}


} // namespace internal

HAS_METHOD(classInfo);

template<typename To, typename From>
To* meta_cast(From *from)
{
    using has_class_info_t = typename has_method_classInfo<ClassInfo(From::*)() const>::type;
    return internal::meta_cast<To, From>(from, has_class_info_t{});
}

template<typename To, typename From>
To const* meta_cast(const From *from)
{
    using has_class_info_t = typename has_method_classInfo<ClassInfo(From::*)() const>::type;
    return internal::meta_cast<To, From>(from, has_class_info_t{});
}

template<typename To, typename From>
To& meta_cast(From &from)
{
    auto ptr = meta_cast<To>(&from);
    if (!ptr)
        throw bad_meta_cast{"Bad meta cast"};
    return *ptr;
}

template<typename To, typename From>
const To& meta_cast(const From &from)
{
    auto ptr = meta_cast<To>(&from);
    if (!ptr)
        throw bad_meta_cast{"Bad meta cast"};
    return *ptr;
}

} // namespace rtti

#endif // METACLASS_H

