#ifndef METATYPE_P_H
#define METATYPE_P_H

#include "metatype.h"
#include "global.h"

namespace rtti {

class MetaClass;

struct DLL_LOCAL TypeInfo {
    const char *name;
    std::size_t nameLength;
    const unsigned int size;
    const MetaType_ID type;
    const MetaType_ID decay;
    const MetaType::TypeFlags flags;
    MetaClass *metaClass = nullptr;

    TypeInfo(const char *name, unsigned int size,
             MetaType_ID type, MetaType_ID decay,
             MetaType::TypeFlags flags)
        : name{name},
          nameLength{std::strlen(name)},
          size{size},
          type{type},
          decay{decay},
          flags{flags}
    {}
};

} // namespace rtti

#endif // METATYPE_P

