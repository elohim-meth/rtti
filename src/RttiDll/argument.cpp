#include "argument.h"
#include "variant.h"

namespace rtti {

argument::argument(const variant &value) noexcept
    : m_data{value.raw_data_ptr()}, m_typeId{value.typeId()}
{}

} //namespace rtti
