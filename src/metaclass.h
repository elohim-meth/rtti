#ifndef METACLASS_H
#define METACLASS_H

#include "metacontainer.h"
#include "metatype.h"
#include "metaerror.h"

#include <sfinae.h>

namespace rtti {

namespace internal {
template<typename To, typename From>
To const* meta_cast(From const*, std::true_type);
} // namespace internal

class MetaClassPrivate;

class DLL_PUBLIC MetaClass final: public MetaContainer
{
    DECLARE_PRIVATE(MetaClass)
public:
    using cast_func_t = void const*(*)(void const*);

    MetaCategory category() const override;
    static MetaClass const* find(MetaType_ID typeId);
    static MetaClass const* find(std::string_view const &name);
    MetaType_ID metaTypeId() const;
    std::size_t baseClassCount() const;
    MetaClass const* baseClass(std::size_t index) const;
    std::size_t derivedClassCount() const;
    MetaClass const* derivedClass(std::size_t index) const;
    bool inheritedFrom(MetaClass const *base) const;
protected:
    explicit MetaClass(std::string_view const &name, MetaContainer const &owner, MetaType_ID typeId);
    static MetaClass* create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId);

    void addBaseClass(MetaType_ID typeId, cast_func_t caster);
    void addDerivedClass(MetaType_ID typeId);
    void const* cast(MetaClass const *base, void const *instance) const;
    void* cast(MetaClass const *base, void *instance) const;

    MetaMethod const* getMethodInternal(std::string_view const &name) const override;
    MetaProperty const* getPropertyInternal(std::string_view const &name) const override;

private:
    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
    DECLARE_ACCESS_KEY(CastAccessKey)
        friend class rtti::variant;

        template<typename To, typename From>
        friend To const* internal::meta_cast(From const*, std::true_type);
    };

public:
    static MetaClass* create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId, CreateAccessKey)
    { return create(name, owner, typeId); }
    void addBaseClass(MetaType_ID typeId, cast_func_t caster, CreateAccessKey)
    { addBaseClass(typeId, caster); }

    void const* cast(MetaClass const *base, void const *instance, CastAccessKey) const
    { return cast(base, instance); }
    void* cast(MetaClass const *base, void *instance, CastAccessKey) const
    { return cast(base, instance); }
};

struct DLL_PUBLIC ClassInfo
{
    MetaType_ID typeId;
    void const *instance = nullptr;

    constexpr ClassInfo() = default;
    constexpr ClassInfo(MetaType_ID typeId, void const *instance)
        : typeId(typeId), instance(instance)
    {}
};

#ifdef __clang__

#define DECLARE_CLASSINFO \
public: \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-override\"") \
    virtual rtti::ClassInfo classInfo() const \
    { \
        return {rtti::metaTypeId<typename std::decay<decltype(*this)>::type>(), this}; \
    } \
    _Pragma("clang diagnostic pop") \
private:

#else

#define DECLARE_CLASSINFO \
public: \
    virtual rtti::ClassInfo classInfo() const \
    { \
        return {rtti::metaTypeId<typename std::decay<decltype(*this)>::type>(), this}; \
    } \
private:

#endif

namespace internal {

template<typename To, typename From>
To const* meta_cast(From const *from, std::true_type)
{
    static_assert(std::is_class_v<From> && std::is_class_v<To>,
                  "Both template arguments should be classes");
    if (!from)
        return nullptr;

    auto const &info = from->classInfo();
    auto *fromClass = MetaClass::find(info.typeId);
    auto *toClass = MetaClass::find(metaTypeId<To>());
    if (!fromClass || !toClass)
        return nullptr;

    auto result = fromClass->cast(toClass, info.instance, {});
    if (!result)
        return nullptr;
    return static_cast<To const*>(result);
}

template<typename To, typename From>
To* meta_cast(From *from, std::true_type)
{
    return const_cast<To*>(meta_cast<To>(const_cast<From const*>(from), std::true_type{}));
}

template<typename To, typename From>
To const* meta_cast(From const *from, std::false_type)
{
    return from;
}

template<typename To, typename From>
To* meta_cast(From *from, std::false_type)
{
    return from;
}


} // namespace internal

HAS_METHOD(classInfo)

template<typename To, typename From>
To* meta_cast(From *from)
{
    using has_class_info_t = typename has_method_classInfo<ClassInfo(From::*)() const>::type;
    return internal::meta_cast<To, From>(from, has_class_info_t{});
}

template<typename To, typename From>
To const* meta_cast(From const *from)
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
To const& meta_cast(From const &from)
{
    auto ptr = meta_cast<To>(&from);
    if (!ptr)
        throw bad_meta_cast{"Bad meta cast"};
    return *ptr;
}

} // namespace rtti

#endif // METACLASS_H

