#ifndef METANAMESPACE_H
#define METANAMESPACE_H

#include "metacontainer.h"

namespace rtti {

class MetaNamespacePrivate;

class DLL_PUBLIC MetaNamespace final: public MetaContainer
{
public:
    static const MetaNamespace* global();
    bool isGlobal() const;
    MetaCategory category() const override;
protected:
    using MetaContainer::MetaContainer;
    static MetaNamespace* create(const char *name, MetaContainer &owner);
private:
    MetaNamespace(); //global namespace

    DECLARE_PRIVATE(MetaNamespace)
    template<typename, typename> friend class rtti::meta_define;
};

} // namespace rtti

#endif // METANAMESPACE_H

