#ifndef METANAMESPACE_P_H
#define METANAMESPACE_P_H

#include "metacontainer_p.h"

#include <rtti/metanamespace.h>
#include <rtti/defines.h>

namespace rtti {

class RTTI_PRIVATE MetaNamespacePrivate: public MetaContainerPrivate
{
public:
    using MetaContainerPrivate::MetaContainerPrivate;
private:
    friend class rtti::MetaNamespace;
};

} //namespace rtti

#endif // METANAMESPACE_P_H

