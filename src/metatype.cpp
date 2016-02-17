#include "metatype_p.h"
#include "metatype.h"

#include <ostream>
#include <mutex>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <cassert>

namespace rtti {

namespace {

#define DEFINE_STATIC_TYPE_INFO(NAME, TYPEID) \
TypeInfo{CString{#NAME}, sizeof(NAME), \
         MetaType_ID{TYPEID}, MetaType_ID{TYPEID}, \
         pointer_arity<NAME>::value, \
         const_bitset<NAME>::value, \
         internal::type_flags<NAME>::value},

static constexpr std::array<TypeInfo, 38> fundamentalTypes = {{
    TypeInfo{CString{"void"}, 0, MetaType_ID{0}, MetaType_ID{0}, 0, 0, internal::type_flags<void>::value},
    FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_TYPE_INFO)
    }};

#undef DEFINE_STATIC_TYPE_INFO

class DLL_LOCAL CustomTypes {
public:
    CustomTypes();
    CustomTypes(CustomTypes const&) = delete;
    CustomTypes& operator=(CustomTypes const&) = delete;
    CustomTypes(CustomTypes &&) = delete;
    CustomTypes& operator=(CustomTypes &&) = delete;
    ~CustomTypes();

    TypeInfo const* getTypeInfo(MetaType_ID typeId) const;
    TypeInfo const*  getTypeInfo(char const *name) const;
    TypeInfo const* addTypeInfo(char const *name, std::size_t size,
                                MetaType_ID decay, std::uint16_t arity,
                                std::uint16_t const_mask, MetaType::TypeFlags flags);
private:
    mutable std::mutex m_lock;
    std::vector<std::unique_ptr<TypeInfo>> m_items;
    std::unordered_map<CString, std::size_t> m_names;
};


#define DEFINE_STATIC_TYPE_MAP(NAME, INDEX) \
{CString{#NAME}, INDEX},

CustomTypes::CustomTypes()
    : m_names {
          {CString{"void"}, 0},
          FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_TYPE_MAP)
}
{}

#undef DEFINE_STATIC_TYPE_MAP

CustomTypes::~CustomTypes()
{
    std::lock_guard<std::mutex> lock{m_lock};
    m_items.clear();
    m_names.clear();
}

TypeInfo const* CustomTypes::getTypeInfo(MetaType_ID typeId) const
{
    auto type = typeId.value();
    if (type == MetaType::InvalidTypeId)
        return nullptr;

    if (type < fundamentalTypes.size())
        return &fundamentalTypes[type];
    type -= fundamentalTypes.size();

    std::lock_guard<std::mutex> lock{m_lock};
    if (type < m_items.size())
        return m_items[type].get();

    return nullptr;
}

const TypeInfo* CustomTypes::getTypeInfo(char const *name) const
{
    if (!name)
        return nullptr;

    auto temp = CString{name};
    std::lock_guard<std::mutex> lock{m_lock};
    auto search = m_names.find(temp);
    if (search != std::end(m_names))
    {
        auto index = search->second;
        if (index < fundamentalTypes.size())
            return &fundamentalTypes[index];

        index -= fundamentalTypes.size();
        if (index < m_items.size())
            return m_items[index].get();
    }
    return nullptr;
}

inline TypeInfo const* CustomTypes::addTypeInfo(char const *name, std::size_t size,
                                            MetaType_ID decay, uint16_t arity,
                                            uint16_t const_mask, MetaType::TypeFlags flags)
{
    std::lock_guard<std::mutex> lock{m_lock};
    auto type = static_cast<MetaType_ID::type>(
                fundamentalTypes.size() + m_items.size());

    // this means that decay_t<type> = type
    if (decay.value() == MetaType::InvalidTypeId)
        decay = MetaType_ID{type};

    auto temp = CString{name};
    auto result = new TypeInfo{temp, size, MetaType_ID{type},
                               decay, arity, const_mask, flags};
    m_items.emplace_back(result);
    m_names.emplace(std::move(temp), type);
    return result;
}

static inline CustomTypes& customTypes()
{
    static CustomTypes result;
    return result;
}

} // namespace

//--------------------------------------------------------------------------------------------------------------------------------
// MetaType
//--------------------------------------------------------------------------------------------------------------------------------

MetaType::MetaType(MetaType_ID typeId)
    : m_typeInfo{customTypes().getTypeInfo(typeId)}
{}

rtti::MetaType::MetaType(char const *name)
    : m_typeInfo(customTypes().getTypeInfo(name))
{}

MetaType_ID MetaType::typeId() const
{
    if (m_typeInfo)
        return m_typeInfo->type;
    return MetaType_ID{};
}

MetaType_ID MetaType::decayId() const
{
    if (m_typeInfo)
        return m_typeInfo->decay;
    return MetaType_ID{};
}

void MetaType::setTypeId(MetaType_ID typeId)
{
    *this = MetaType{typeId};
}

char const* MetaType::typeName() const
{
    if (m_typeInfo)
        return m_typeInfo->name.data();
    return nullptr;
}

std::size_t MetaType::typeSize() const
{
    if (m_typeInfo)
        return m_typeInfo->size;
    return 0;
}

MetaType::TypeFlags MetaType::typeFlags() const
{
    MetaType::TypeFlags result = TypeFlags::None;
    if (m_typeInfo)
        result = m_typeInfo->flags;
    return result;
}

std::uint16_t MetaType::pointerArity() const
{
    if (m_typeInfo)
        return m_typeInfo->arity;
    return 0;
}

bool MetaType::compatible(MetaType fromType, MetaType toType)
{
    if (!fromType.valid() || !toType.valid())
        return false;

    if (!fromType.isLvalueReference() && toType.isLvalueReference() && !toType.isConst())
        return false;
    if (fromType.isLvalueReference() && toType.isRvalueReference())
        return false;

    // check array length
    if (fromType.isArray() && toType.isArray() && (fromType.typeSize() < toType.typeSize()))
        return false;

    // decay array to pointer
    auto arity1 = fromType.m_typeInfo->arity;
    if (fromType.isArray() && toType.isPointer())
        ++arity1;

    // compare pointer arity
    if (arity1 != toType.m_typeInfo->arity)
        return false;

    // skip top-most const when destination type isn't reference
    if (!toType.isReference())
    {
        if (!arity1)
            return true;
        --arity1;
    }

    auto from = TypeInfo::const_bitset_t{fromType.m_typeInfo->const_mask};
    auto to = TypeInfo::const_bitset_t{toType.m_typeInfo->const_mask};

    // Decay to reference to pointer should be reference to const pointer
    if (fromType.isArray() && !toType.isArray() && toType.isReference())
        from.set(arity1);

    if (from == to)
        return true;

    for (decltype(arity1) i = 0; i <= arity1; ++i)
    {
        auto f = from.test(i);
        auto t = to.test(i);

        if (f && !t)
            return false;

        if (!f & t)
        {
            for (decltype(arity1) j = i + 1; j <= arity1; ++j)
                if (!to.test(j))
                    return false;
            break;
        }
    }
    return true;
}

MetaType_ID MetaType::registerMetaType(char const *name, std::size_t size,
                                       MetaType_ID decay, std::uint16_t arity,
                                       std::uint16_t const_mask,
                                       MetaType::TypeFlags flags)
{
    auto result = customTypes().addTypeInfo(name, size, decay, arity, const_mask, flags);
    return result->type;
}

//--------------------------------------------------------------------------------------------------------------------------------
// Converter
//--------------------------------------------------------------------------------------------------------------------------------

namespace {

template<typename F>
class MetaTypeFunctionList
{
public:
    using key_t = std::pair<MetaType_ID, MetaType_ID>;

    ~MetaTypeFunctionList()
    {
        std::lock_guard<std::mutex> lock{m_lock};
        m_items.clear();
        Destroyed = true;
    }

    bool find(key_t const &key) const
    {
        std::lock_guard<std::mutex> lock{m_lock};
        return (m_items.find(key) != std::end(m_items));
    }

    bool add(key_t key, F const *func)
    {
        if (!func)
            return false;

        std::lock_guard<std::mutex> lock{m_lock};
        auto search = m_items.find(key);
        if (search != std::end(m_items))
            return false;

        m_items.emplace(key, func);
        return true;
    }

    F const* get(key_t const &key) const
    {
        std::lock_guard<std::mutex> lock{m_lock};
        auto search = m_items.find(key);
        if (search != std::end(m_items))
            return search->second;
        return nullptr;
    }

    void remove(key_t const &key)
    {
        std::lock_guard<std::mutex> lock{m_lock};
        m_items.erase(key);
    }
private:
    struct hash_key
    {
        using result_type = std::size_t;
        using argument_type = key_t;
        result_type operator()(argument_type const &key) const
        {
            return std::_Hash_impl::__hash_combine(key.first.value(),
                        std::_Hash_impl::hash(key.second.value()));
        }
    };

    mutable std::mutex m_lock;
    std::unordered_map<key_t, F const*, hash_key> m_items;

    static bool Destroyed;

    friend MetaTypeFunctionList<internal::ConvertFunctionBase>* customConverters();
};

template<typename F>
bool MetaTypeFunctionList<F>::Destroyed = false;

inline MetaTypeFunctionList<internal::ConvertFunctionBase>* customConverters()
{
    if (MetaTypeFunctionList<internal::ConvertFunctionBase>::Destroyed)
        return nullptr;

    static MetaTypeFunctionList<internal::ConvertFunctionBase> result;
    return &result;
}

} //namespace

bool MetaType::hasConverter(MetaType fromType, MetaType toType)
{
    if (fromType.valid() && toType.valid())
    {
        auto list = customConverters();
        if (list)
            return list->find({fromType.m_typeInfo->decay,
                               toType.m_typeInfo->decay});
    }
    return false;
}

bool MetaType::hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId)
{
    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    return hasConverter(fromType, toType);
}

bool MetaType::registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                 internal::ConvertFunctionBase const &converter)
{
    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
    {
        auto list = customConverters();
        if (list)
            return list->add({fromType.m_typeInfo->decay,
                              toType.m_typeInfo->decay},
                             &converter);
    }
    return false;
}

bool MetaType::convert(void const *from, MetaType fromType, void *to, MetaType toType)
{
    if (fromType.valid() && toType.valid())
    {
        auto list = customConverters();
        if (list)
        {
            auto converter = list->get({fromType.m_typeInfo->decay,
                                        toType.m_typeInfo->decay});
            assert(converter);
            return converter->invoke(from, to);
        }
    }
    return false;
}

bool MetaType::convert(void const *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId)
{
    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    return convert(from, fromType, to, toType);
}

void MetaType::unregisterConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId)
{
    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
    {
        auto list = customConverters();
        if (list)
            list->remove({fromType.m_typeInfo->decay,
                          toType.m_typeInfo->decay});
    }
}

} //namespace rtti

std::ostream& operator<<(std::ostream &stream, rtti::MetaType const &value)
{
    return stream << value.typeId().value() << ":"
                  << value.typeName() << ":"
                  << value.typeFlags();

}
