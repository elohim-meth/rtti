#include "metatype_p.h"
#include "metatype.h"

#include <ostream>
#include <mutex>
#include <vector>
#include <array>
#include <unordered_map>

namespace rtti {

namespace {

#define DEFINE_STATIC_TYPE_INFO(NAME, TYPEID) \
TypeInfo{CString{#NAME}, sizeof(NAME), MetaType_ID{TYPEID}, MetaType_ID{TYPEID}, type_flags<NAME>::value},

static constexpr std::array<TypeInfo, 41> fundamentalTypes = {
    TypeInfo{CString{"void"}, 0, MetaType_ID{0}, MetaType_ID{0}, type_flags<void>::value},
    FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_STATIC_TYPE_INFO)
    };

#undef DEFINE_STATIC_TYPE_INFO

class CustomTypes {
public:
    CustomTypes();
    CustomTypes(const CustomTypes &) = delete;
    CustomTypes& operator=(const CustomTypes &) = delete;
    CustomTypes(CustomTypes &&) = delete;
    CustomTypes& operator=(CustomTypes &&) = delete;
    ~CustomTypes() noexcept;

    const TypeInfo* getTypeInfo(MetaType_ID typeId) const;
    const TypeInfo* getTypeInfo(const char *name) const;
    MetaType_ID addTypeInfo(const char *name, unsigned int size,
                            MetaType_ID decay, MetaType::TypeFlags flags);
private:
    mutable std::mutex m_lock;
    std::vector<TypeInfo> m_items;
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

CustomTypes::~CustomTypes() noexcept
{
    std::lock_guard<std::mutex> lock{m_lock};
    m_items.clear();
    m_names.clear();
}

const TypeInfo* CustomTypes::getTypeInfo(MetaType_ID typeId) const
{
    auto type = typeId.value();
    if (type == MetaType::InvalidTypeId)
        return nullptr;

    if (type < fundamentalTypes.size())
        return &fundamentalTypes[type];
    type -= fundamentalTypes.size();

    std::lock_guard<std::mutex> lock{m_lock};
    if (type < m_items.size())
        return &m_items[type];

    return nullptr;
}

const TypeInfo* CustomTypes::getTypeInfo(const char *name) const
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
            return &m_items[index];
    }
    return nullptr;
}

inline MetaType_ID CustomTypes::addTypeInfo(const char *name, unsigned int size,
                                            MetaType_ID decay, MetaType::TypeFlags flags)
{
    std::lock_guard<std::mutex> lock{m_lock};
    MetaType_ID::type result = fundamentalTypes.size() + m_items.size();

    // this means that decay_t<type> = type
    if (decay.value() == MetaType::InvalidTypeId)
        decay = MetaType_ID{result};

    auto temp = CString{name};
    m_items.emplace_back(temp, size, MetaType_ID{result}, decay, flags);
    m_names.emplace(std::move(temp), result);
    return MetaType_ID{result};
}

static inline CustomTypes& customTypes()
{
    static CustomTypes result;
    return result;
}

} // namespace internal

//--------------------------------------------------------------------------------------------------------------------------------
// MetaType
//--------------------------------------------------------------------------------------------------------------------------------

MetaType::MetaType(MetaType_ID typeId)
    : m_typeInfo(customTypes().getTypeInfo(typeId))
{}

rtti::MetaType::MetaType(const char *name)
    : m_typeInfo(customTypes().getTypeInfo(name))
{}

MetaType_ID MetaType::typeId() const noexcept
{
    if (m_typeInfo)
        return m_typeInfo->type;
    return MetaType_ID{};
}

void MetaType::setTypeId(MetaType_ID typeId)
{
    *this = MetaType{typeId};
}

const char* MetaType::typeName() const noexcept
{
    if (m_typeInfo)
        return m_typeInfo->name.data();
    return nullptr;
}

std::size_t MetaType::typeSize() const noexcept
{
    if (m_typeInfo)
        return m_typeInfo->size;
    return 0;
}

MetaType::TypeFlags MetaType::typeFlags() const noexcept
{
    MetaType::TypeFlags result = TypeFlags::None;
    if (m_typeInfo)
        result = m_typeInfo->flags;
    return result;
}

MetaType_ID MetaType::registerMetaType(const char *name, unsigned int size,
                                       MetaType_ID decay,
                                       MetaType::TypeFlags flags)
{
    return customTypes().addTypeInfo(name, size, decay, flags);
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

    ~MetaTypeFunctionList() noexcept
    {
        std::lock_guard<std::mutex> lock{m_lock};
        m_items.clear();
    }

    bool find(const key_t &key) const
    {
        std::lock_guard<std::mutex> lock{m_lock};
        return (m_items.find(key) != std::end(m_items));
    }

    bool add(key_t key, const F *func)
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

    const F* get(const key_t &key) const
    {
        std::lock_guard<std::mutex> lock{m_lock};
        auto search = m_items.find(key);
        if (search != std::end(m_items))
            return search->second;
        return nullptr;
    }

    void remove(const key_t &key)
    {
        std::lock_guard<std::mutex> lock{m_lock};
        m_items.erase(key);
    }
private:
    struct hash_key
    {
        using result_type = std::size_t;
        using argument_type = key_t;
        result_type operator()(const argument_type &key) const
        {
            return std::_Hash_impl::__hash_combine(key.first.value(),
                        std::_Hash_impl::hash(key.second.value()));
        }
    };

    mutable std::mutex m_lock;
    std::unordered_map<key_t, const F*, hash_key> m_items;
};

static inline MetaTypeFunctionList<internal::ConvertFunctionBase>& customConverters()
{
    static MetaTypeFunctionList<internal::ConvertFunctionBase> result;
    return result;
}

} //namespace

bool MetaType::hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId)
{
    if (fromTypeId.value() == InvalidTypeId || toTypeId.value() == InvalidTypeId)
        return false;

    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
        return customConverters().find({fromType.m_typeInfo->decay,
                                        toType.m_typeInfo->decay});
    return false;
}

bool MetaType::registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                 const internal::ConvertFunctionBase &converter)
{
    if (fromTypeId.value() == InvalidTypeId || toTypeId.value() == InvalidTypeId)
        return false;

    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
        return customConverters().add({fromType.m_typeInfo->decay,
                                       toType.m_typeInfo->decay},
                                      &converter);
    return false;
}

bool MetaType::convert(const void *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId)
{
    if (fromTypeId.value() == InvalidTypeId || toTypeId.value() == InvalidTypeId)
        return false;

    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
    {
        auto converter = customConverters().get({fromType.m_typeInfo->decay,
                                                 toType.m_typeInfo->decay});
        if (converter)
            return converter->invoke(from, to);
    }

    return false;
}

void MetaType::unregisterConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId)
{
    if (fromTypeId.value() == InvalidTypeId || toTypeId.value() == InvalidTypeId)
        return;

    auto fromType = MetaType{fromTypeId};
    auto toType = MetaType{toTypeId};
    if (fromType.valid() && toType.valid())
        customConverters().remove({fromType.m_typeInfo->decay,
                                   toType.m_typeInfo->decay});
}

} //namespace rtti

std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value)
{
    return stream << value.typeId().value() << ":"
                  << value.typeName() << ":"
                  << value.typeFlags();

}
