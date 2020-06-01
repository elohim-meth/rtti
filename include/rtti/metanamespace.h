#ifndef METANAMESPACE_H
#define METANAMESPACE_H

#include <rtti/metacontainer.h>

namespace rtti {

class MetaNamespacePrivate;

class RTTI_API MetaNamespace final: public MetaContainer
{
    DECLARE_PRIVATE(MetaNamespace)
public:
    static MetaNamespace const* global();
    bool isGlobal() const;
    MetaCategory category() const override;
protected:
    using MetaContainer::MetaContainer;
    static MetaNamespace* create(std::string_view name, MetaContainer &owner);
private:
    MetaNamespace(); //global namespace

    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    static MetaNamespace* create(std::string_view name, MetaContainer &owner, CreateAccessKey)
    { return create(name, owner); }
};

} // namespace rtti

#endif // METANAMESPACE_H

