#include <rtti/metamethod.h>

namespace rtti {

argument::~argument() noexcept
{
    if (m_buffer)
    {
        m_type.destroy(m_buffer);
        m_type.deallocate(m_buffer);
    }
}

MetaType_ID argument::typeId() const
{
    if (isVariant())
    {
        auto *v = static_cast<variant*>(m_data);
        if (m_type.isLvalueReference() && m_type.isConst())
            return v->internalTypeId(variant::type_attribute::LREF_CONST, {});
        else if (m_type.isLvalueReference())
            return v->internalTypeId(variant::type_attribute::LREF, {});
        else
            return v->internalTypeId(variant::type_attribute::NONE, {});
    }
    return m_type.typeId();
}

bool argument::isVariant() const
{
    return (m_type.decayId() == metaTypeId<variant>());
}

bool argument::empty() const noexcept
{
    return m_data == nullptr;
}

} // namespace rtti
