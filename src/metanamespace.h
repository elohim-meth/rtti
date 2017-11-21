#ifndef METANAMESPACE_H
#define METANAMESPACE_H

#include "metacontainer.h"

namespace rtti {

class MetaNamespacePrivate;

class DLL_PUBLIC MetaNamespace final: public MetaContainer
{
    DECLARE_PRIVATE(MetaNamespace)
public:
    static MetaNamespace const* global();
    bool isGlobal() const;
    MetaCategory category() const override;
protected:
    using MetaContainer::MetaContainer;
    static MetaNamespace* create(const char *name, MetaContainer &owner);
private:
    MetaNamespace(); //global namespace

    DECLARE_ACCESS_KEY(CreateAccessKey)
        template<typename, typename> friend class rtti::meta_define;
    };
public:
    static MetaNamespace* create(const char *name, MetaContainer &owner, CreateAccessKey)
    { return create(name, owner); }
};

} // namespace rtti

#endif // METANAMESPACE_H

