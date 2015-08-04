﻿#ifndef METATYPE_P_H
#define METATYPE_P_H

#include "metatype.h"

#include <c_string.h>

#include "global.h"

namespace rtti {

struct DLL_LOCAL TypeInfo {
    const CString name;
    const std::size_t size;
    const MetaType_ID type;
    const MetaType_ID decay;
    const std::uint8_t arity;
    const std::uint8_t const_mask;
    const MetaType::TypeFlags flags;
    MetaClass *metaClass = nullptr;

    constexpr TypeInfo(CString name, std::size_t size,
                       MetaType_ID type, MetaType_ID decay,
                       std::uint8_t arity, std::uint8_t const_mask,
                       MetaType::TypeFlags flags)
        : name{std::move(name)},
          size{size}, type{type},
          decay{decay}, arity{arity},
          const_mask{const_mask},
          flags{flags}
    {}
};

} // namespace rtti

#endif // METATYPE_P
