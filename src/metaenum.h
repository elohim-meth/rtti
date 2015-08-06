#ifndef METAENUM_H
#define METAENUM_H

#include "metaitem.h"
#include "metatype.h"

#include <functional>

namespace rtti {

class MetaEnumPrivate;

class DLL_PUBLIC MetaEnum final: public MetaItem
{
public:
    using enum_element_t = std::function<bool(const std::string&, const variant&)>;

    MetaCategory category() const override;

    std::size_t elementCount() const;
    const variant& element(std::size_t index) const;
    const std::string& elementName(std::size_t index) const;
    const variant& element(const char *name) const;
    void for_each_element(const enum_element_t &func) const;
protected:
    explicit MetaEnum(const char *name, const MetaContainer &owner, MetaType_ID typeId);
    static MetaEnum* create(const char *name, MetaContainer &owner, MetaType_ID typeId);

    void addElement(const char *name, variant &&value);
private:
    DECLARE_PRIVATE(MetaEnum)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METAENUM_H

