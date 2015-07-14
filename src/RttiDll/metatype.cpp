#include "metatype_p.h"
#include "metatype.h"

#include <ostream>
#include <mutex>
#include <vector>
#include <cstring>

namespace rtti {

namespace {

#define FUNDAMENTAL_TYPE_INFO(NAME, TYPEID) \
{#NAME, sizeof(NAME), MetaType_ID{TYPEID}, type_flags<NAME>::value},

static const std::vector<TypeInfo> fundamentalTypes = {
    {"void", 0, MetaType_ID{0}, type_flags<void>::value},
    FUNDAMENTAL_TYPE_INFO(bool, 1)
    FUNDAMENTAL_TYPE_INFO(char, 2)
    FUNDAMENTAL_TYPE_INFO(signed char, 3)
    FUNDAMENTAL_TYPE_INFO(unsigned char, 4)
    FUNDAMENTAL_TYPE_INFO(short, 5)
    FUNDAMENTAL_TYPE_INFO(unsigned short, 6)
    FUNDAMENTAL_TYPE_INFO(int, 7)
    FUNDAMENTAL_TYPE_INFO(unsigned int, 8)
    FUNDAMENTAL_TYPE_INFO(long, 9)
    FUNDAMENTAL_TYPE_INFO(unsigned long, 10)
    FUNDAMENTAL_TYPE_INFO(long long, 11)
    FUNDAMENTAL_TYPE_INFO(unsigned long long, 12)
    FUNDAMENTAL_TYPE_INFO(float, 13)
    FUNDAMENTAL_TYPE_INFO(double, 14)
    FUNDAMENTAL_TYPE_INFO(long double, 15)
    FUNDAMENTAL_TYPE_INFO(char16_t, 16)
    FUNDAMENTAL_TYPE_INFO(char32_t, 17)
    FUNDAMENTAL_TYPE_INFO(wchar_t, 18)
    //
    FUNDAMENTAL_TYPE_INFO(void*, 19)
    FUNDAMENTAL_TYPE_INFO(bool*, 20)
    FUNDAMENTAL_TYPE_INFO(char*, 21)
    FUNDAMENTAL_TYPE_INFO(signed char*, 22)
    FUNDAMENTAL_TYPE_INFO(unsigned char*, 23)
    FUNDAMENTAL_TYPE_INFO(short*, 24)
    FUNDAMENTAL_TYPE_INFO(unsigned short*, 25)
    FUNDAMENTAL_TYPE_INFO(int*, 26)
    FUNDAMENTAL_TYPE_INFO(unsigned int*, 27)
    FUNDAMENTAL_TYPE_INFO(long*, 28)
    FUNDAMENTAL_TYPE_INFO(unsigned long*, 29)
    FUNDAMENTAL_TYPE_INFO(long long*, 30)
    FUNDAMENTAL_TYPE_INFO(unsigned long long*, 31)
    FUNDAMENTAL_TYPE_INFO(float*, 32)
    FUNDAMENTAL_TYPE_INFO(double*, 33)
    FUNDAMENTAL_TYPE_INFO(long double*, 34)
    FUNDAMENTAL_TYPE_INFO(char16_t*, 35)
    FUNDAMENTAL_TYPE_INFO(char32_t*, 36)
    FUNDAMENTAL_TYPE_INFO(wchar_t*, 37)
    //
    FUNDAMENTAL_TYPE_INFO(const void*, 38)
    FUNDAMENTAL_TYPE_INFO(const char*, 39)
    FUNDAMENTAL_TYPE_INFO(const wchar_t*, 40)
    };

#undef FUNDAMENTAL_TYPE_INFO

class CustomTypes {
public:
    CustomTypes() = default;
    CustomTypes(const CustomTypes &) = delete;
    CustomTypes& operator=(const CustomTypes &) = delete;
    CustomTypes(CustomTypes &&) = delete;
    CustomTypes& operator=(CustomTypes &&) = delete;
    ~CustomTypes() = default;

    const TypeInfo* getTypeInfo(MetaType_ID typeId) const;
    const TypeInfo* getTypeInfo(const char *name) const;
    MetaType_ID addTypeInfo(const char *name, unsigned int size,
                            MetaType::TypeFlags flags);
private:
    mutable std::mutex m_lock;
    std::vector<TypeInfo> m_list;
};

const TypeInfo* CustomTypes::getTypeInfo(MetaType_ID typeId) const
{
    auto type = typeId.value();
    if (type == MetaType::InvalidTypeId)
        return nullptr;

    if (type < fundamentalTypes.size())
        return &fundamentalTypes[type];
    type -= fundamentalTypes.size();

    std::lock_guard<std::mutex> lock{m_lock};
    if (type < m_list.size())
        return &m_list[type];

    return nullptr;
}

const TypeInfo* CustomTypes::getTypeInfo(const char *name) const
{
    if (!name || (strnlen(name, 1) == 0))
        return nullptr;

    auto length = std::strlen(name);
    for (const auto &item: fundamentalTypes)
    {
        if ((item.nameLength == length) && (std::strncmp(name, item.name, length) == 0))
            return &item;
    }

    std::lock_guard<std::mutex> lock{m_lock};
    for (const auto &item: m_list)
    {
        if ((item.nameLength == length) && (std::strncmp(name, item.name, length) == 0))
            return &item;
    }

    return nullptr;
}

inline MetaType_ID CustomTypes::addTypeInfo(const char *name, unsigned int size, MetaType::TypeFlags flags)
{
    std::lock_guard<std::mutex> lock{m_lock};
    MetaType_ID::type result = fundamentalTypes.size() + m_list.size();
    m_list.emplace_back(name, size, MetaType_ID{result}, flags);
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
    auto result = MetaType_ID{InvalidTypeId};
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
        result = m_typeInfo->name;
    return result;
}

MetaType::TypeFlags MetaType::typeFlags() const noexcept
{
    MetaType::TypeFlags result = TypeFlags::None;
    if (m_typeInfo)
        result = m_typeInfo->flags;
    return result;
}

MetaType_ID MetaType::registerMetaType(const char *name, unsigned int size, MetaType::TypeFlags flags)
{
    return customTypes().addTypeInfo(name, size, flags);
}

} //namespace rtti

std::ostream& operator<<(std::ostream &stream, const rtti::MetaType &value)
{
    return stream << value.typeId().value() << ":"
                  << value.typeName() << ":"
                  << value.typeFlags();

}
