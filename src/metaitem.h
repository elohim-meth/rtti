#ifndef METAITEM_H
#define METAITEM_H

#include "global.h"

#include <memory>

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr.get()); } \
    friend class rtti::Class##Private;

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
public:
    using enum_attribute_t = std::function<bool(const std::string&, const variant&)>;

    virtual MetaCategory category() const noexcept = 0;
    const std::string& name() const;
    const MetaContainer* owner() const noexcept;
    const std::string& qualifiedName() const;
    std::size_t attributeCount() const noexcept;
    const variant& attribute(std::size_t index) const noexcept;
    const std::string& attributeName(std::size_t index) const noexcept;
    const variant& attribute(const char *name) const;
    void for_each_attribute(const enum_attribute_t &func) const;
protected:
    explicit MetaItem(const char *name, const MetaContainer &owner);
    explicit MetaItem(MetaItemPrivate &value);
    MetaItem(const MetaItem &) = delete;
    MetaItem& operator=(const MetaItem &) = delete;
    MetaItem(MetaItem &&) = delete;
    MetaItem& operator=(MetaItem &&) = delete;
    virtual ~MetaItem();

    virtual void checkDeferredDefine() const;

    void setAttribute(const char *name, const variant &value);
    void setAttribute(const char *name, variant &&value);
    std::unique_ptr<MetaItemPrivate> d_ptr;

private:
    DECLARE_PRIVATE(MetaItem)
    friend class rtti::internal::MetaItemList;
    template<typename, typename> friend class rtti::meta_define;
};

} //namespace rtti

#endif // METAITEM_H
