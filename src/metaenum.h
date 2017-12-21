#ifndef METAENUM_H
#define METAENUM_H

#include "metaitem.h"
#include "metatype.h"

#include <functional>

namespace rtti {

class MetaEnumPrivate;

class DLL_PUBLIC MetaEnum final: public MetaItem
{
    DECLARE_PRIVATE(MetaEnum)
public:
    using enum_element_t = std::function<bool(std::string const&, variant const&)>;

    MetaCategory category() const override;

    std::size_t elementCount() const;
    variant const& element(std::size_t index) const;
    std::string const& elementName(std::size_t index) const;
    variant const& element(std::string_view const &name) const;
    void for_each_element(enum_element_t const &func) const;
protected:
    explicit MetaEnum(std::string_view const &name, const MetaContainer &owner, MetaType_ID typeId);
    static MetaEnum* create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId);

    void addElement(std::string_view const &name, variant &&value);
private:
    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    static MetaEnum* create(std::string_view const &name, MetaContainer &owner, MetaType_ID typeId, CreateAccessKey)
    { return create(name, owner, typeId); }
    void addElement(std::string_view const &name, variant &&value, CreateAccessKey)
    { addElement(name, std::move(value)); }
};

} // namespace rtti

#endif // METAENUM_H

