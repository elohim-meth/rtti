#ifndef METAITEM_H
#define METAITEM_H

#include <rtti/export.h>
#include <rtti/defines.h>

#include <functional>
#include <memory>
#include <string>

namespace rtti {

enum MetaCategory : unsigned int
{
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
class MetaContainer;
class MetaItemPrivate;

template<typename, typename> class meta_define;
namespace internal {
class MetaItemList;
} //namespace internal

class RTTI_API MetaItem
{
    DECLARE_PRIVATE(MetaItem)
public:
    using enum_attribute_t = std::function<bool(std::string_view, variant const&)>;

    virtual MetaCategory category() const = 0;
    std::string const& name() const;
    MetaContainer const* owner() const;
    std::string const& qualifiedName() const;
    std::size_t attributeCount() const;
    variant const& attribute(std::size_t index) const;
    std::string const &attributeName(std::size_t index) const;
    variant const& attribute(std::string_view name) const;
    void for_each_attribute(enum_attribute_t const &func) const;
protected:
    explicit MetaItem(std::string_view name, MetaContainer const &owner);
    explicit MetaItem(MetaItemPrivate &value);

    MetaItem(MetaItem const&)               = delete;
    MetaItem& operator=(MetaItem const&)    = delete;
    MetaItem(MetaItem &&)                   = delete;
    MetaItem& operator=(MetaItem &&)        = delete;
    virtual ~MetaItem();

    virtual void checkDeferredDefine() const;

    void setAttribute(std::string_view name, variant const &value);
    void setAttribute(std::string_view name, variant &&value);
    RTTI_PRIVATE std::unique_ptr<MetaItemPrivate> d_ptr;

private:
    DECLARE_ACCESS_KEY(SetAttributeKey)
        template<typename, typename> friend class rtti::meta_define;
    };
    friend class rtti::internal::MetaItemList;
public:
    void setAttribute(std::string_view name, variant const &value, SetAttributeKey)
    { setAttribute(name, value); }
    void setAttribute(std::string_view name, variant &&value, SetAttributeKey)
    { setAttribute(name, std::move(value)); }
};

} //namespace rtti

#endif // METAITEM_H
