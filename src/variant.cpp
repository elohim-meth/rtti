#include <rtti/variant.h>

namespace rtti {

variant const variant::empty_variant = {};

variant::variant() noexcept
    : manager(internal::variant_function_table_for<void>())
{}

variant::variant(variant const &other)
    : manager{other.manager}
{
    manager->f_copy(other.storage, storage);
}

variant& variant::operator=(variant const &other)
{
    if (this != &other)
        variant{other}.swap(*this);
    return *this;
}

variant::variant(variant &&other) noexcept
    : manager(internal::variant_function_table_for<void>())
{
    swap(other);
}

variant& variant::operator=(variant &&other) noexcept
{
    if (this != &other)
        variant{std::move(other)}.swap(*this);
    return *this;
}

variant::~variant() noexcept
{
    clear();
}

void variant::swap(variant &other) noexcept
{
    using std::swap;

    auto thisEmpty  = this->empty();
    auto otherEmpty = other.empty();
    if (thisEmpty && otherEmpty)
        return;

    if (thisEmpty && !otherEmpty)
    {
        other.manager->f_move(other.storage, storage);
        manager = other.manager;

        other.manager = internal::variant_function_table_for<void>();
        other.storage = storage_t{};
        return;
    }

    if (!thisEmpty && otherEmpty)
    {
        manager->f_move(storage, other.storage);
        other.manager = manager;

        manager = internal::variant_function_table_for<void>();
        storage = storage_t{};
        return;
    }

    storage_t temporary;
    manager->f_move(storage, temporary);
    other.manager->f_move(other.storage, storage);
    manager->f_move(temporary, other.storage);

    swap(manager, other.manager);
}

void variant::clear() noexcept
{
    manager->f_destroy(storage);
    manager = internal::variant_function_table_for<void>();
    storage = storage_t{};
}

bool variant::empty() const noexcept
{
    return (manager == internal::variant_function_table_for<void>());
}

bool variant::operator==(variant const &value) const
{
    if (empty() && value.empty())
        return true;
    if (empty() || value.empty())
        return false;

    auto mt_left  = MetaType{this->internalTypeId(type_attribute::LREF_CONST)};
    auto mt_right = MetaType{value.internalTypeId(type_attribute::LREF_CONST)};

    if (MetaType::compatible(mt_right, mt_left))
    {
        if (mt_right.decayId() == mt_left.decayId())
        {
            auto ptr = value.raw_data_ptr();
            if (mt_right.isArray())
                ptr = *reinterpret_cast<void const *const *>(ptr);

            return manager->f_compare_eq(storage, ptr);
        }
    }

    return false;
}

variant variant::get_property(std::string_view name) const
{
    using namespace std::literals;

    auto type = MetaType{typeId()};
    if (type.isClass() || type.isClassPtr())
    {
        if (auto *mt_class = this->metaClass())
        {
            if (auto *mt_property = mt_class->getProperty(name))
                return mt_property->get(*this);

            throw runtime_error{"Property ["s + name + "] isn't found in class T = " + type.typeName()};
        }
        throw runtime_error{"Class T = "s + type.typeName() + " isn't registered"};
    }
    throw runtime_error{"Type T = "s + type.typeName() + " isn't Class or ClassPtr"};
}

} // namespace rtti
