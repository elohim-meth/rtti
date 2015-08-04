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
    storage = {.buffer = {0}};
}

void variant::swap(variant &other) noexcept
{
    storage_t temporary = {.buffer = {0}};
    manager->f_move(storage, temporary);
    other.manager->f_move(other.storage, storage);
    manager->f_move(temporary, other.storage);

    std::swap(manager, other.manager);
}

void variant::clear() noexcept
{
    variant{}.swap(*this);
}

bool variant::empty() const noexcept
{
    return manager == internal::function_table_for<void>();
}

variant::operator bool() const noexcept
{
    return !empty();
}

MetaType_ID variant::typeId() const noexcept
{
    return manager->f_type();
}

ClassInfo variant::classInfo() const noexcept
{
    return manager->f_info(storage);
}

void* variant::raw_data_ptr() const noexcept
{
    return manager->f_access(storage);
}

} // namespace rtti
