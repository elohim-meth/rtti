#ifndef METATYPE_P_H
#define METATYPE_P_H

#include <bitset>

#include "metatype.h"

#include <c_string.h>

#include "global.h"

namespace rtti {

struct DLL_LOCAL TypeInfo {
    using const_bitset_t = std::bitset<16>;

    CString const name;
    std::size_t const size;
    MetaType_ID const type;
    MetaType_ID const decay;
    std::uint16_t const arity;
    std::uint16_t const const_mask;
    TypeFlags const flags;
    metatype_manager_t const *manager;

    mutable MetaClass *metaClass = nullptr;

    constexpr TypeInfo(CString name, std::size_t size,
                       MetaType_ID type, MetaType_ID decay,
                       std::uint16_t arity, std::uint16_t const_mask,
                       TypeFlags flags, metatype_manager_t const *manager)
        : name{std::move(name)},
          size{size}, type{type},
          decay{decay}, arity{arity},
          const_mask{const_mask},
          flags{flags}, manager{manager}
    {}
};

} // namespace rtti

#endif // METATYPE_P

