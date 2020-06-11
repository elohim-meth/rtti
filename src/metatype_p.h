#ifndef METATYPE_P_H
#define METATYPE_P_H

#include <bitset>
#include <atomic>

#include <rtti/metatype.h>
#include <rtti/defines.h>

namespace rtti {

struct RTTI_PRIVATE TypeInfo
{
    using const_bitset_t = std::bitset<16>;

    std::string_view const name;
    std::size_t const size;
    MetaType_ID const decay;
    std::uint16_t const arity;
    std::uint16_t const const_mask;
    TypeFlags const flags;
    metatype_manager_t const *manager;

    mutable std::atomic<MetaClass *> metaClass = nullptr;

    constexpr TypeInfo(std::string_view name, std::size_t size, MetaType_ID decay,
                       std::uint16_t arity, std::uint16_t const_mask, TypeFlags flags,
                       metatype_manager_t const *manager)
        : name{name}
        , size{size}
        , decay{decay.valid() ? decay : MetaType_ID{reinterpret_cast<MetaType_ID::type>(this)}}
        , arity{arity}
        , const_mask{const_mask}
        , flags{flags}
        , manager{manager}
    {}
};

} // namespace rtti

#endif // METATYPE_P
