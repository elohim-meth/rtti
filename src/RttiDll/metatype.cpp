#include "metatype_p.h"
#include "metatype.h"

#include <ostream>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace rtti {

namespace {

#define DEFINE_TYPE_INFO(NAME, TYPEID) \
{CString{#NAME}, sizeof(NAME), MetaType_ID{TYPEID}, MetaType_ID{TYPEID}, type_flags<NAME>::value},

static const std::vector<TypeInfo> fundamentalTypes = {
    {CString{"void"}, 0, MetaType_ID{0}, MetaType_ID{0}, type_flags<void>::value},
    FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_TYPE_INFO)
    };

#undef DEFINE_TYPE_INFO

class CustomTypes {
public:
    CustomTypes();
    CustomTypes(const CustomTypes &) = delete;
    CustomTypes& operator=(const CustomTypes &) = delete;
    CustomTypes(CustomTypes &&) = delete;
    CustomTypes& operator=(CustomTypes &&) = delete;
    ~CustomTypes() = default;

    const TypeInfo* getTypeInfo(MetaType_ID typeId) const;
    const TypeInfo* getTypeInfo(const char *name) const;
    MetaType_ID addTypeInfo(const char *name, unsigned int size,
                            MetaType_ID decay, MetaType::TypeFlags flags);
private:
    mutable std::mutex m_lock;
    std::vector<TypeInfo> m_items;
    std::unordered_map<CString, std::size_t> m_names;
};


#define DEFINE_TYPE_MAP(NAME, INDEX) \
{CString{#NAME}, INDEX},

CustomTypes::CustomTypes()
    : m_names {
          {CString{"void"}, 0},
          FOR_EACH_FUNDAMENTAL_TYPE(DEFINE_TYPE_MAP)
      }
{}

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

static inline CustomTypes& customTypes() {
    static CustomTypes result;
    return result;
}

}

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
    auto result = MetaType_ID{};
    if (m_typeInfo)
        result = m_typeInfo->type;
    return result;
}

void MetaType::setTypeId(MetaType_ID typeId)
{
    *this = MetaType{typeId};
}

const char* MetaType::typeName() const noexcept
{
    const char *result = nullptr;
    if (m_typeInfo)
        result = m_typeInfo->name.data();
    return result;
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

} //namespace rtti

std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value)
{
    return stream << value.typeId().value() << ":"
                  << value.typeName() << ":"
                  << value.typeFlags();

}
