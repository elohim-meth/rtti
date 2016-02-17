#include "argument.h"

namespace rtti {

MetaType_ID argument::typeId() const
{
    auto type = MetaType{m_typeId};
    if (type.decayId() == metaTypeId<variant>())
    {
        auto *v = static_cast<variant*>(m_data);
        if (type.isLvalueReference() && type.isConst())
            return v->internalTypeId(variant::type_attribute::LREF_CONST);
        else if (type.isLvalueReference())
            return v->internalTypeId(variant::type_attribute::LREF);
        else
            return v->internalTypeId(variant::type_attribute::NONE);

    }
    return m_typeId;
}

}
