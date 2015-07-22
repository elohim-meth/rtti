#ifndef METACLASS_H
#define METACLASS_H

#include "metacontainer.h"
#include "metatype.h"
#include "metaerror.h"

#include <sfinae.h>

namespace rtti {

namespace internal {
template<typename To, typename From>
To* meta_cast_selector(const From*, std::true_type);
} // namespace internal

class MetaClassPrivate;

class DLL_PUBLIC MetaClass final: public MetaContainer
{
public:
    using cast_func_t = void*(*)(void*);

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

    void addBaseClass(MetaType_ID typeId, cast_func_t caster);
    void addDerivedClass(MetaType_ID typeId);
    void* cast(const MetaClass *base, const void *instance) const;
private:
    DECLARE_PRIVATE(MetaClass)
    template<typename, typename> friend class rtti::meta_define;
    template<typename To, typename From>
    friend To* internal::meta_cast_selector(const From*, std::true_type);
    friend class rtti::variant;
};

struct DLL_PUBLIC ClassInfo
{
    MetaType_ID typeId;
    const void* instance = nullptr;

    constexpr ClassInfo() noexcept = default;
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
To* meta_cast_selector(const From *from, std::true_type)
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
    return static_cast<To*>(result);
}

template<typename To, typename From>
To* meta_cast_selector(const From *from, std::false_type)
{
    return const_cast<From*>(from);
}

} // namespace internal

HAS_METHOD(classInfo);

template<typename To, typename From>
To* meta_cast(From *from)
{
    return internal::meta_cast_selector<To, From>(
                from, typename has_method_classInfo<ClassInfo(From::*)() const>::type{});
}

template<typename To, typename From>
const To* meta_cast(const From *from)
{
    return internal::meta_cast_selector<To, From>(
                from, typename has_method_classInfo<ClassInfo(From::*)() const>::type{});
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

