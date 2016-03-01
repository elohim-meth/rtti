#include "variant.h"

namespace rtti {

const variant variant::empty_variant = {};

variant::variant(const variant &other)
    : manager{other.manager}
{
    manager->f_copy(other.storage, storage);
}

variant& variant::operator=(const variant &other)
{
    if (this != &other)
        variant{other}.swap(*this);
    return *this;
}

variant::variant(variant &&other) noexcept
{
    swap(other);
}

variant& variant::operator=(variant &&other) noexcept
{
    variant{std::move(other)}.swap(*this);
    return *this;
}

variant::~variant() noexcept
{
    clear();
}

void variant::swap(variant &other) noexcept
{
    auto thisEmpty = this->empty();
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

    std::swap(manager, other.manager);
}

void variant::clear() noexcept
{
    manager->f_destroy(storage);
    manager = internal::variant_function_table_for<void>();
    storage = storage_t{};
}

bool variant::empty() const noexcept
{
    return manager == internal::variant_function_table_for<void>();
}

} // namespace rtti
