#ifndef METATYPE_P_H
#define METATYPE_P_H

#include "metatype.h"
#include "c_string.h"
#include "global.h"

namespace rtti {

class MetaClass;

struct DLL_LOCAL TypeInfo {
    const CString name;
    const unsigned int size;
    const MetaType_ID type;
    const MetaType_ID decay;
    const MetaType::TypeFlags flags;
    MetaClass *metaClass = nullptr;

    constexpr TypeInfo(CString name, unsigned int size, MetaType_ID type, MetaType_ID decay, MetaType::TypeFlags flags)
        : name{std::move(name)},
          size{size},
          type{type},
          decay{decay},
          flags{flags}
    {}
};

} // namespace rtti

#endif // METATYPE_P

