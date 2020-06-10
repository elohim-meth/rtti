#ifndef METAENUM_P_H
#define METAENUM_P_H

#include "metaitem_p.h"

#include <rtti/metaenum.h>

namespace rtti {

class RTTI_PRIVATE MetaEnumPrivate: public MetaItemPrivate
{
public:
    MetaEnumPrivate(std::string_view name, MetaContainer const &owner, MetaType_ID typeId)
        : MetaItemPrivate{name, owner}
        , m_typeId{typeId}
    {}

private:
    MetaType_ID m_typeId;
    internal::NamedVariantList m_elements;

    friend class rtti::MetaEnum;
};

} // namespace rtti

#endif // METAENUM_P_H
