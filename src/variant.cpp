#include "variant.h"

namespace rtti {

const variant variant::empty_variant = {};

variant::variant(const variant &other)
    : manager{other.manager}
{
    manager->f_clone(other.storage, storage);
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

variant::~variant()
{
    manager->f_destroy(storage);
    manager = internal::function_table_for<void>();
    storage = storage_t{};
}

void variant::swap(variant &other) noexcept
{
    storage_t temporary;
    manager->f_move(storage, temporary);
    other.manager->f_move(other.storage, storage);
    manager->f_move(temporary, other.storage);

    std::swap(manager, other.manager);
}

bool variant::empty() const
{
    return manager == internal::function_table_for<void>();
}

} // namespace rtti
