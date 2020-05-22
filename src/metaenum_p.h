#ifndef METAENUM_P_H
#define METAENUM_P_H

#include <rtti/metaenum.h>
#include "metaitem_p.h"

namespace rtti {

class RTTI_PRIVATE MetaEnumPrivate: public MetaItemPrivate
{
public:
    MetaEnumPrivate(std::string_view const &name, const MetaContainer &owner, MetaType_ID typeId)
        : MetaItemPrivate{name, owner}, m_typeId{typeId}
    {}

private:
    MetaType_ID m_typeId;
    internal::NamedVariantList m_elements;

    friend class rtti::MetaEnum;
};

} // namespace rtti

#endif // METAENUM_P_H

