#include "argument.h"

namespace rtti {

bool argument::empty() const noexcept
{
    return m_data == nullptr;
}

MetaType_ID argument::typeId() const noexcept
{
    return m_typeId;
}

} //namespace rtti
