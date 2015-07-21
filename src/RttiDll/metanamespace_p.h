#ifndef METANAMESPACE_P_H
#define METANAMESPACE_P_H

#include "metanamespace.h"
#include "metacontainer_p.h"
#include "global.h"

namespace rtti {

class DLL_LOCAL MetaNamespacePrivate: public MetaContainerPrivate
{
public:
    using MetaContainerPrivate::MetaContainerPrivate;
private:
    friend class rtti::MetaNamespace;
};

} //namespace rtti

#endif // METANAMESPACE_P_H

