#include "argument.h"

namespace rtti {

bool argument::empty() const
{
    return m_data == nullptr;
}

MetaType_ID argument::typeId() const
{
    return m_typeId;
}

} //namespace rtti
