﻿#ifndef METAITEM_H
#define METAITEM_H

#include "global.h"

#include <memory>

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline Class##Private const* d_func() const { return reinterpret_cast<Class##Private const*>(d_ptr.get()); } \
    friend class rtti::Class##Private;

#define DECLARE_ACCESS_KEY(NAME) \
  class NAME  \
  { \
      NAME() {}; \
      NAME(NAME const&) = default;

namespace rtti {

enum MetaCategory: int {
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

class DLL_PUBLIC MetaItem
{
    DECLARE_PRIVATE(MetaItem)
public:
    using enum_attribute_t = std::function<bool(std::string const&, variant const&)>;

    virtual MetaCategory category() const = 0;
    std::string const& name() const;
    MetaContainer const* owner() const;
    std::string const& qualifiedName() const;
    std::size_t attributeCount() const;
    variant const& attribute(std::size_t index) const;
    std::string const& attributeName(std::size_t index) const;
    variant const& attribute(char const *name) const;
    void for_each_attribute(enum_attribute_t const &func) const;
protected:
    explicit MetaItem(char const *name, MetaContainer const &owner);
    explicit MetaItem(MetaItemPrivate &value);
    MetaItem(MetaItem const&) = delete;
    MetaItem& operator=(MetaItem const&) = delete;
    MetaItem(MetaItem &&) = delete;
    MetaItem& operator=(MetaItem &&) = delete;
    virtual ~MetaItem();

    virtual void checkDeferredDefine() const;

    void setAttribute(char const *name, variant const &value);
    void setAttribute(char const *name, variant &&value);
    std::unique_ptr<MetaItemPrivate> d_ptr;

private:
    DECLARE_ACCESS_KEY(SetAttributeKey)
        template<typename, typename> friend class rtti::meta_define;
    };
    friend class rtti::internal::MetaItemList;
public:
    void setAttribute(char const *name, variant const &value, SetAttributeKey)
    { setAttribute(name, value); }
    void setAttribute(char const *name, variant &&value, SetAttributeKey)
    { setAttribute(name, std::move(value)); }
};

} //namespace rtti

#endif // METAITEM_H
