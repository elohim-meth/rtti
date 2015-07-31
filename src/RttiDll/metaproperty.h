#ifndef METAPROPERTY_H
#define METAPROPERTY_H

#include "metaitem.h"
#include "metatype.h"
#include "argument.h"

namespace rtti {

struct DLL_PUBLIC IPropertyInvoker
{
    virtual bool isStatic() const = 0;
    virtual MetaType_ID typeId() const = 0;
    virtual variant get_static() const = 0;
    virtual void set_static(argument arg) const = 0;
    virtual variant get_field(const variant &instance) const = 0;
    virtual void set_field(const variant &instance, argument arg) const = 0;
    virtual ~IPropertyInvoker() noexcept = default;
};

class MetaPropertyPrivate;

class DLL_PUBLIC MetaProperty final: public MetaItem
{
public:
    MetaCategory category() const noexcept override;
protected:
    explicit MetaProperty(const char *name, MetaContainer &owner,
                        std::unique_ptr<IPropertyInvoker> invoker);
    static MetaProperty* create(const char *name, MetaContainer &owner,
                              std::unique_ptr<IPropertyInvoker> invoker);
private:
    const IPropertyInvoker* invoker() const noexcept;

    DECLARE_PRIVATE(MetaProperty)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METAPROPERTY_H

