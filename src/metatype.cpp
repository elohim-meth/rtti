#include "metatype_p.h"

#include <rtti/metatype.h>
#include <rtti/signature.h>

#include <ostream>
#include <shared_mutex>
#include <forward_list>
#include <unordered_map>
#include <memory>
#include <cassert>

namespace rtti {

namespace {

class RTTI_PRIVATE CustomTypes
{
public:
    CustomTypes();
    CustomTypes(CustomTypes const &) = delete;
    CustomTypes &operator=(CustomTypes const &) = delete;
    CustomTypes(CustomTypes &&)                 = delete;
    CustomTypes &operator=(CustomTypes &&) = delete;
    ~CustomTypes();

    inline MetaType_ID getTypeId(TypeInfo const *type_info) const;
    inline TypeInfo const *getTypeInfo(MetaType_ID typeId) const;
    TypeInfo const *getTypeInfo(std::string_view name) const;
    TypeInfo const *addTypeInfo(std::string_view name, std::size_t size, MetaType_ID decay,
                                std::uint16_t arity, std::uint16_t const_mask, TypeFlags flags,
                                metatype_manager_t const *manager);

private:
    mutable std::shared_mutex m_lock;
    std::forward_list<TypeInfo> m_items;
    std::unordered_map<std::string_view, TypeInfo const *> m_names;

    static bool Destroyed;
    friend CustomTypes *customTypes();
};

bool CustomTypes::Destroyed = false;

CustomTypes::CustomTypes() {}

CustomTypes::~CustomTypes()
{
    std::unique_lock lock{m_lock};
    m_items.clear();
    m_names.clear();
    Destroyed = true;
}

inline MetaType_ID CustomTypes::getTypeId(TypeInfo const *type_info) const
{
    return MetaType_ID{reinterpret_cast<MetaType_ID::type>(type_info)};
}

inline TypeInfo const* CustomTypes::getTypeInfo(MetaType_ID typeId) const
{
    return reinterpret_cast<TypeInfo const *>(typeId.value());
}

TypeInfo const* CustomTypes::getTypeInfo(std::string_view name) const
{
    if (name.empty())
        return nullptr;

    std::shared_lock lock{m_lock};
    if (auto it = m_names.find(name); it != std::end(m_names))
        return it->second;

    return nullptr;
}

TypeInfo const* CustomTypes::addTypeInfo(std::string_view name, std::size_t size, MetaType_ID decay,
                                         uint16_t arity, uint16_t const_mask, TypeFlags flags,
                                         metatype_manager_t const *manager)
{
    if (name.empty())
        return nullptr;

    std::unique_lock lock{m_lock};
    if (auto it = m_names.find(name); it == std::end(m_names))
    {
        auto &result = m_items.emplace_front(name, size, decay, arity, const_mask, flags, manager);
        m_names.emplace(name, &result);
        return &result;
    }
    else
        return it->second;
}

inline CustomTypes* customTypes()
{
    if (CustomTypes::Destroyed)
        return nullptr;

    static CustomTypes result;
    return &result;
}

} // namespace

//--------------------------------------------------------------------------------------------------------------------------------
// MetaType
//--------------------------------------------------------------------------------------------------------------------------------

MetaType::MetaType(MetaType_ID typeId) noexcept
{
    if (auto *types = customTypes())
        m_typeInfo = types->getTypeInfo(typeId);
}

MetaType::MetaType(std::string_view name) noexcept
{
    if (auto *types = customTypes())
        m_typeInfo = types->getTypeInfo(name);

}

MetaType_ID MetaType::typeId() const noexcept
{
    if (auto *types = customTypes())
        return types->getTypeId(m_typeInfo);

    return MetaType_ID{};
}

MetaType_ID MetaType::decayId() const noexcept
{
    return m_typeInfo ? m_typeInfo->decay
                      : MetaType_ID{};
}

std::string_view MetaType::typeName() const noexcept
{
    using namespace std::string_view_literals;
    return m_typeInfo ? m_typeInfo->name
                      : "void"sv;
}

std::size_t MetaType::typeSize() const noexcept
{
    return m_typeInfo ? m_typeInfo->size
                      : 0;
}

TypeFlags MetaType::typeFlags() const noexcept
{
    return m_typeInfo ? m_typeInfo->flags
                      : TypeFlags::Void;
}

std::uint16_t MetaType::pointerArity() const noexcept
{
    return m_typeInfo ? m_typeInfo->arity
                      : 0;
}

bool MetaType::compatible(MetaType fromType, MetaType toType) noexcept
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
    auto to   = TypeInfo::const_bitset_t{toType.m_typeInfo->const_mask};

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

MetaType_ID MetaType::registerMetaType(std::string_view name, std::size_t size, MetaType_ID decay,
                                       std::uint16_t arity, std::uint16_t const_mask,
                                       TypeFlags flags, metatype_manager_t const *manager)
{
    auto *types = customTypes();
    if (!types)
        return MetaType_ID{};

    auto result = types->addTypeInfo(name, size, decay, arity, const_mask, flags, manager);
    return types->getTypeId(result);
}

MetaClass const* MetaType::metaClass() const noexcept
{
    return m_typeInfo ? m_typeInfo->metaClass.load()
                      : nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------------
// Low level construction
//--------------------------------------------------------------------------------------------------------------------------------

void* MetaType::allocate() const
{
    return m_typeInfo ? m_typeInfo->manager->f_allocate()
                      : nullptr;
}

void MetaType::deallocate(void *ptr) const
{
    if (m_typeInfo)
        m_typeInfo->manager->f_deallocate(ptr);
}

void MetaType::default_construct(void *where) const
{
    if (m_typeInfo)
        m_typeInfo->manager->f_default_construct(where);
}

void MetaType::copy_construct(void const *source, void *where) const
{
    if (m_typeInfo)
        m_typeInfo->manager->f_copy_construct(source, where);
}

void MetaType::move_construct(void *source, void *where) const
{
    if (m_typeInfo)
        m_typeInfo->manager->f_move_construct(source, where);
}

void MetaType::move_or_copy(void *source, void *where) const
{
    if (m_typeInfo)
        m_typeInfo->manager->f_move_or_copy(source, where);
}

void MetaType::destroy(void *ptr) const noexcept
{
    if (m_typeInfo)
        m_typeInfo->manager->f_destroy(ptr);
}

bool MetaType::compare_eq(void const *lhs, void const *rhs) const
{
    return m_typeInfo ? m_typeInfo->manager->f_compare_eq(lhs, rhs)
                      : false;
}

void* MetaType::construct(void *copy, bool movable) const
{
    auto result = allocate();
    if (!copy)
        default_construct(result);
    else if (!movable)
        copy_construct(copy, result);
    else
        move_or_copy(copy, result);

    return result;
}

void MetaType::destruct(void *instance) const
{
    if (instance)
    {
        destroy(instance);
        deallocate(instance);
    }
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
            std::unique_lock lock{m_lock};
            m_items.clear();
            Destroyed = true;
        }

        bool find(key_t const &key) const
        {
            std::shared_lock lock{m_lock};
            return (m_items.find(key) != std::end(m_items));
        }

        bool add(key_t key, F const *func)
        {
            if (!func)
                return false;

            std::unique_lock lock{m_lock};
            if (auto search = m_items.find(key); search != std::end(m_items))
                return false;

            m_items.emplace(key, func);
            return true;
        }

        F const* get(key_t const &key) const
        {
            std::shared_lock lock{m_lock};
            if (auto search = m_items.find(key); search != std::end(m_items))
                return search->second;
            return nullptr;
        }

        void remove(key_t const &key)
        {
            std::unique_lock lock{m_lock};
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

        mutable std::shared_mutex m_lock;
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

bool MetaType::hasConverter(MetaType fromType, MetaType toType) noexcept
{
    if (fromType.valid() && toType.valid())
        if (auto list = customConverters())
            return list->find({fromType.m_typeInfo->decay, toType.m_typeInfo->decay});
    return false;
}

bool MetaType::hasConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId) noexcept
{
    auto fromType = MetaType{fromTypeId};
    auto toType   = MetaType{toTypeId};

    return hasConverter(fromType, toType);
}

bool MetaType::registerConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId,
                                 internal::ConvertFunctionBase const &converter)
{
    auto fromType = MetaType{fromTypeId};
    auto toType   = MetaType{toTypeId};

    if (fromType.valid() && toType.valid())
        if (auto list = customConverters())
            return list->add({fromType.m_typeInfo->decay, toType.m_typeInfo->decay}, &converter);
    return false;
}

bool MetaType::convert(void const *from, MetaType fromType, void *to, MetaType toType)
{
    if (fromType.valid() && toType.valid())
        if (auto list = customConverters())
        {
            auto converter = list->get({fromType.m_typeInfo->decay, toType.m_typeInfo->decay});
            if (converter)
                return converter->invoke(from, to);
        }
    return false;
}

bool MetaType::convert(void const *from, MetaType_ID fromTypeId, void *to, MetaType_ID toTypeId)
{
    auto fromType = MetaType{fromTypeId};
    auto toType   = MetaType{toTypeId};

    return convert(from, fromType, to, toType);
}

void MetaType::unregisterConverter(MetaType_ID fromTypeId, MetaType_ID toTypeId)
{
    auto fromType = MetaType{fromTypeId};
    auto toType   = MetaType{toTypeId};

    if (fromType.valid() && toType.valid())
        if (auto list = customConverters())
            list->remove({fromType.m_typeInfo->decay, toType.m_typeInfo->decay});
}

static char const *flag_names[] = {"None",

                                   "Const",
                                   "Pointer",
                                   "MemberPointer",
                                   "LvalueReference",
                                   "RvalueReference",
                                   "Array",

                                   "Void",
                                   "Integral",
                                   "FloatPoint",
                                   "Enum",
                                   "Function",
                                   "Union",
                                   "Class",

                                   "StandardLayout",
                                   "Trivial",
                                   "Abstract",
                                   "Polymorphic",
                                   "DefaultConstructible",
                                   "CopyConstructible",
                                   "MoveConstructible",
                                   "MoveAssignable",
                                   "Destructible",

                                   "EQ_Comparable"};

inline static std::string_view flag_name(TypeFlags value)
{
    return flag_names[static_cast<std::underlying_type_t<TypeFlags>>(value)];
}

inline static constexpr bool check_flag(TypeFlags value, TypeFlags bit)
{
    return ((value & bit) == bit);
}

std::ostream& operator<<(std::ostream &stream, TypeFlags value)
{
    auto it = prefix_ostream_iterator<std::string_view>{stream, "|"};
    if (value == TypeFlags::None)
    {
        it = flag_name(TypeFlags::None);
        return stream;
    }

    if (check_flag(value, TypeFlags::LvalueReference))
        it = flag_name(TypeFlags::LvalueReference);
    if (check_flag(value, TypeFlags::RvalueReference))
        it = flag_name(TypeFlags::RvalueReference);
    if (check_flag(value, TypeFlags::Pointer))
        it = flag_name(TypeFlags::Pointer);
    if (check_flag(value, TypeFlags::MemberPointer))
        it = flag_name(TypeFlags::MemberPointer);
    if (check_flag(value, TypeFlags::Const))
        it = flag_name(TypeFlags::Const);
    if (check_flag(value, TypeFlags::Array))
        it = flag_name(TypeFlags::Array);

    if (check_flag(value, TypeFlags::Void))
        it = flag_name(TypeFlags::Void);
    if (check_flag(value, TypeFlags::Integral))
        it = flag_name(TypeFlags::Integral);
    if (check_flag(value, TypeFlags::FloatPoint))
        it = flag_name(TypeFlags::FloatPoint);
    if (check_flag(value, TypeFlags::Enum))
        it = flag_name(TypeFlags::Enum);
    if (check_flag(value, TypeFlags::Function))
        it = flag_name(TypeFlags::Function);
    if (check_flag(value, TypeFlags::Union))
        it = flag_name(TypeFlags::Union);
    if (check_flag(value, TypeFlags::Class))
        it = flag_name(TypeFlags::Class);

    if (check_flag(value, TypeFlags::StandardLayout))
        it = flag_name(TypeFlags::StandardLayout);
    if (check_flag(value, TypeFlags::Trivial))
        it = flag_name(TypeFlags::Trivial);
    if (check_flag(value, TypeFlags::Abstract))
        it = flag_name(TypeFlags::Abstract);
    if (check_flag(value, TypeFlags::Polymorphic))
        it = flag_name(TypeFlags::Polymorphic);

    if (check_flag(value, TypeFlags::DefaultConstructible))
        it = flag_name(TypeFlags::DefaultConstructible);
    if (check_flag(value, TypeFlags::CopyConstructible))
        it = flag_name(TypeFlags::CopyConstructible);
    if (check_flag(value, TypeFlags::MoveConstructible))
        it = flag_name(TypeFlags::MoveConstructible);
    if (check_flag(value, TypeFlags::MoveAssignable))
        it = flag_name(TypeFlags::MoveAssignable);
    if (check_flag(value, TypeFlags::Destructible))
        it = flag_name(TypeFlags::Destructible);

    if (check_flag(value, TypeFlags::EQ_Comparable))
        it = flag_name(TypeFlags::EQ_Comparable);

    return stream;
}

std::ostream& operator<<(std::ostream &stream, MetaType value)
{
    return stream << "ID: [" << value.typeId().value() << "], "
                  << "Name: [" << value.typeName() << "], "
                  << "Flags: [" << value.typeFlags() << "]";
}

} //namespace rtti

